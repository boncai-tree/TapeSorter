#include "tatlin/file_tape.hpp"
#include "tatlin/file_tape_factory.hpp"
#include "test_utils.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <filesystem>
#include <limits>
#include <vector>

namespace {

using tatlin::test::read_int32_file;
using tatlin::test::TempDir;
using tatlin::test::write_int32_file;

TEST(FileTapeUtils, Int32CellsInFile_EmptyFile) {
  TempDir dir;
  std::filesystem::path p = dir.path / "empty.bin";
  write_int32_file(p, {});
  EXPECT_EQ(tatlin::int32_cells_in_file(p), 0U);
}

TEST(FileTapeUtils, Int32CellsInFile_CountsInts) {
  TempDir dir;
  std::filesystem::path p = dir.path / "a.bin";
  write_int32_file(p, {1, 2, -3, 0});
  EXPECT_EQ(tatlin::int32_cells_in_file(p), 4U);
}

TEST(FileTapeUtils, Int32CellsInFile_UnalignedSizeThrowsTapeError) {
  TempDir dir;
  std::filesystem::path p = dir.path / "bad.bin";
  {
    std::ofstream out(p, std::ios::binary | std::ios::trunc);
    ASSERT_TRUE(static_cast<bool>(out));
    char buf[3] = {1, 2, 3};
    out.write(buf, 3);
  }
  EXPECT_THROW(static_cast<void>(tatlin::int32_cells_in_file(p)), tatlin::TapeError);
}

TEST(FileTape, EmptyTapeAtBeginAndEnd) {
  TempDir dir;
  std::filesystem::path const p = dir.path / "e.bin";
  write_int32_file(p, {});
  tatlin::FileTape tape(p, tatlin::AppConfig{}, false);
  EXPECT_TRUE(tape.at_begin());
  EXPECT_TRUE(tape.at_end());
}

TEST(FileTape, ReadWriteRoundTrip) {
  TempDir dir;
  std::filesystem::path p = dir.path / "tape.bin";
  write_int32_file(p, {10, 20, 30});

  tatlin::AppConfig cfg;
  tatlin::FileTape tape(p, cfg, false);

  EXPECT_TRUE(tape.at_begin());
  EXPECT_FALSE(tape.at_end());

  EXPECT_EQ(tape.read(), 10);
  tape.shift_next();
  EXPECT_EQ(tape.read(), 20);
  tape.shift_next();
  EXPECT_EQ(tape.read(), 30);
  EXPECT_FALSE(tape.at_end());
  tape.shift_next();
  EXPECT_TRUE(tape.at_end());
}

TEST(FileTape, Rewind) {
  TempDir dir;
  std::filesystem::path p = dir.path / "tape.bin";
  write_int32_file(p, {1, 2});
  tatlin::FileTape tape(p, tatlin::AppConfig{}, false);
  EXPECT_EQ(tape.read(), 1);
  tape.shift_next();
  tape.rewind();
  EXPECT_TRUE(tape.at_begin());
  EXPECT_EQ(tape.read(), 1);
}

TEST(FileTape, ShiftNextPastEndThrowsBounds) {
  TempDir dir;
  std::filesystem::path p = dir.path / "tape.bin";
  write_int32_file(p, {7});
  tatlin::FileTape tape(p, tatlin::AppConfig{}, false);
  EXPECT_EQ(tape.read(), 7);
  ASSERT_FALSE(tape.at_end());
  tape.shift_next();
  ASSERT_TRUE(tape.at_end());
  EXPECT_THROW(tape.shift_next(), tatlin::TapeBoundsError);
}

TEST(FileTape, ShiftPrevBeforeBeginThrowsBounds) {
  TempDir dir;
  std::filesystem::path p = dir.path / "tape.bin";
  write_int32_file(p, {1});
  tatlin::FileTape tape(p, tatlin::AppConfig{}, false);
  EXPECT_THROW(tape.shift_prev(), tatlin::TapeBoundsError);
}

TEST(FileTape, WriteSequenceThenReadRoundTrip) {
  TempDir dir;
  tatlin::AppConfig cfg;
  tatlin::FileTapeFactory factory(cfg, dir.path / "tape_factory_tmp");

  std::filesystem::path p = dir.path / "written.bin";
  std::vector<std::int32_t> const vals =
      {-111, 0, 999, std::numeric_limits<std::int32_t>::min(), std::numeric_limits<std::int32_t>::max()};

  {
    std::unique_ptr<tatlin::Tape> tape = factory.create_tape_by_path(p, vals.size());
    ASSERT_TRUE(tape->at_begin());
    for (std::int32_t v : vals) {
      tape->write(v);
      tape->shift_next();
    }

    tape->rewind();
    for (std::int32_t expected : vals) {
      EXPECT_EQ(tape->read(), expected);
      tape->shift_next();
    }
    ASSERT_TRUE(tape->at_end());
  }

  EXPECT_EQ(read_int32_file(p), vals);
}

TEST(FileTape, SingleCellWriteThenVerifyOnDisk) {
  TempDir dir;
  tatlin::AppConfig cfg;
  tatlin::FileTapeFactory factory(cfg, dir.path / "tape_factory_tmp");

  std::filesystem::path p = dir.path / "one.bin";

  {
    std::unique_ptr<tatlin::Tape> tape = factory.create_tape_by_path(p, 1);
    tape->write(424242);
    ASSERT_FALSE(tape->at_end());
    tape->shift_next();
    ASSERT_TRUE(tape->at_end());
  }

  auto const blob = read_int32_file(p);
  ASSERT_EQ(blob.size(), 1U);
  EXPECT_EQ(blob[0], 424242);
}

} // namespace
