#include "tatlin/errors.hpp"
#include "tatlin/file_tape.hpp"
#include "tatlin/file_tape_factory.hpp"
#include "tatlin/tape_sorter.hpp"
#include "test_utils.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <random>
#include <vector>

namespace {

using tatlin::test::read_int32_file;
using tatlin::test::TempDir;
using tatlin::test::write_int32_file;

tatlin::AppConfig minimal_sort_config(std::size_t memory_limit_bytes, std::size_t temporary_tapes_k) {
  tatlin::AppConfig cfg;
  cfg.memory_limit_bytes = memory_limit_bytes;
  cfg.temporary_tapes_k = temporary_tapes_k;
  return cfg;
}

void sort_validation(
    std::vector<std::int32_t> const& input_values,
    std::size_t memory_limit_bytes = 8192,
    std::size_t temporary_tapes_k = 32
) {
  TempDir dir;
  std::filesystem::path input_path = dir.path / "in.bin";
  std::filesystem::path output_path = dir.path / "out.bin";

  write_int32_file(input_path, input_values);

  tatlin::AppConfig cfg = minimal_sort_config(memory_limit_bytes, temporary_tapes_k);
  tatlin::FileTapeFactory factory(cfg, dir.path / "factory_tmp");

  std::size_t n = tatlin::int32_cells_in_file(input_path);
  auto input_tape = factory.create_tape_from_file(input_path);
  auto output_tape = factory.create_tape_by_path(output_path, n);

  std::size_t ints_capacity = cfg.memory_limit_bytes / sizeof(std::int32_t);
  tatlin::TapeSorter sorter(&factory, ints_capacity, cfg.temporary_tapes_k);

  EXPECT_NO_THROW(sorter.sort(*input_tape, *output_tape, n));

  input_tape.reset();
  output_tape.reset();

  auto result = read_int32_file(output_path);
  ASSERT_EQ(result.size(), input_values.size());

  auto expected = input_values;
  std::ranges::sort(expected);
  EXPECT_EQ(result, expected);
}

TEST(TapeSorter, ConstructorNullFactoryThrows) {
  EXPECT_THROW((tatlin::TapeSorter(nullptr, 512, 32)), tatlin::InvariantError);
}

TEST(TapeSorter, ConstructorIntsCapacityBelowTwoThrowsConfigError) {
  tatlin::FileTapeFactory f(minimal_sort_config(4096, 32), ".");
  EXPECT_THROW((tatlin::TapeSorter(&f, 1, 32)), tatlin::ConfigError);
}

TEST(TapeSorter, SortLeavesEmptyOutputWhenNothingToSort) {
  ASSERT_NO_FATAL_FAILURE(sort_validation({}));
}

TEST(TapeSorter, SortPreservesSingleElement) {
  ASSERT_NO_FATAL_FAILURE(sort_validation({42}));
}

TEST(TapeSorter, SortSmallPermutation) {
  ASSERT_NO_FATAL_FAILURE(sort_validation({5, -1, 3, 3, 0}));
}

TEST(TapeSorter, SortManyElementsWithSmallMemoryForcesMergePasses) {
  std::mt19937 gen(12345);
  std::uniform_int_distribution<> dist(-1000000, 1000000);
  std::vector<std::int32_t> v;
  v.reserve(2000);
  for (std::size_t i = 0; i < 2000; ++i) {
    v.push_back(static_cast<std::int32_t>(dist(gen)));
  }

  ASSERT_NO_FATAL_FAILURE(sort_validation(v, 64, 32));
}

TEST(TapeSorter, SortThrowsWhenTapeBudgetTooSmallDuringSort) {
  TempDir dir;
  tatlin::AppConfig cfg = minimal_sort_config(4096, 32);

  tatlin::FileTapeFactory factory(cfg, dir.path / "factory_tmp");

  std::filesystem::path din = dir.path / "in.bin";
  std::filesystem::path dout = dir.path / "out.bin";

  write_int32_file(din, {10, 9, 8, 7, 6});

  auto input_tape = factory.create_tape_from_file(din);
  std::size_t n = 5;
  auto output_tape = factory.create_tape_by_path(dout, n);

  std::size_t ints_capacity = 2;

  tatlin::TapeSorter sorter(&factory, ints_capacity, 32);

  EXPECT_THROW(sorter.sort(*input_tape, *output_tape, n), tatlin::ConfigError);
}

} // namespace
