#include "tatlin/app_config.hpp"
#include "test_utils.hpp"

#include <gtest/gtest.h>

#include <fstream>

namespace {

using tatlin::test::TempDir;

void write_text_file(std::filesystem::path const& p, char const* text) {
  std::ofstream out(p);
  ASSERT_TRUE(static_cast<bool>(out));
  out << text;
}

TEST(AppConfig, LoadParsesKnownKeys) {
  TempDir dir;
  auto cfg_path = dir.path / "c.ini";
  write_text_file(
      cfg_path,
      "# comment\n"
      "read_delay_ms=1\n"
      "write_delay_ms=2\n"
      "rewind_delay_ms=3\n"
      "shift_delay_ms=4\n"
      "memory_limit_bytes=128\n"
      "temporary_tapes_k=8\n"
  );

  tatlin::AppConfig cfg = tatlin::load_config(cfg_path);
  EXPECT_EQ(cfg.read_delay.count(), 1);
  EXPECT_EQ(cfg.write_delay.count(), 2);
  EXPECT_EQ(cfg.rewind_delay.count(), 3);
  EXPECT_EQ(cfg.shift_delay.count(), 4);
  EXPECT_EQ(cfg.memory_limit_bytes, 128U);
  EXPECT_EQ(cfg.temporary_tapes_k, 8U);
}

TEST(AppConfig, MissingEqualsThrowsConfigError) {
  TempDir dir;
  auto cfg_path = dir.path / "bad.ini";
  write_text_file(cfg_path, "not_a_line\nmemory_limit_bytes=128\ntemporary_tapes_k=4\n");

  EXPECT_THROW(static_cast<void>(tatlin::load_config(cfg_path)), tatlin::ConfigError);
}

TEST(AppConfig, InvalidIntegerThrows) {
  TempDir dir;
  auto cfg_path = dir.path / "bad.ini";
  write_text_file(
      cfg_path,
      "memory_limit_bytes=x\n"
      "temporary_tapes_k=4\n"
  );

  EXPECT_THROW(static_cast<void>(tatlin::load_config(cfg_path)), tatlin::ConfigError);
}

TEST(AppConfig, NegativeDelayThrows) {
  TempDir dir;
  auto cfg_path = dir.path / "bad.ini";
  write_text_file(
      cfg_path,
      "read_delay_ms=-1\n"
      "memory_limit_bytes=128\n"
      "temporary_tapes_k=4\n"
  );

  EXPECT_THROW(static_cast<void>(tatlin::load_config(cfg_path)), tatlin::ConfigError);
}

TEST(AppConfig, MemoryZeroThrows) {
  TempDir dir;
  auto cfg_path = dir.path / "bad.ini";
  write_text_file(
      cfg_path,
      "memory_limit_bytes=0\n"
      "temporary_tapes_k=4\n"
  );

  EXPECT_THROW(static_cast<void>(tatlin::load_config(cfg_path)), tatlin::ConfigError);
}

TEST(AppConfig, TemporaryTapesKTooSmallThrows) {
  TempDir dir;
  auto cfg_path = dir.path / "bad.ini";
  write_text_file(
      cfg_path,
      "memory_limit_bytes=128\n"
      "temporary_tapes_k=1\n"
  );

  EXPECT_THROW(static_cast<void>(tatlin::load_config(cfg_path)), tatlin::ConfigError);
}

TEST(AppConfig, MemoryAllowsLessThanTwoInt32ThrowsAfterParse) {
  TempDir dir;
  auto cfg_path = dir.path / "bad.ini";
  write_text_file(
      cfg_path,
      "memory_limit_bytes=7\n"
      "temporary_tapes_k=4\n"
  );

  EXPECT_THROW(static_cast<void>(tatlin::load_config(cfg_path)), tatlin::ConfigError);
}

} // namespace
