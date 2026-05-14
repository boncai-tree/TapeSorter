#pragma once

#include "app_config.hpp"
#include "tape.hpp"

#include <filesystem>
#include <fstream>

namespace tatlin {

class FileTape final : public Tape {
public:
  ~FileTape() override;

  FileTape(std::filesystem::path path, AppConfig delays, bool delete_on_destroy = false);

  [[nodiscard]] std::int32_t read() override;
  void write(std::int32_t value) override;
  void shift_next() override;
  void shift_prev() override;
  void rewind() override;
  [[nodiscard]] bool at_begin() const override;
  [[nodiscard]] bool at_end() const override;

private:
  void apply_read_delay() const;
  void apply_write_delay() const;
  void apply_rewind_delay() const;
  void apply_shift_delay() const;

  void seek_to_position() const;

  std::filesystem::path path_;
  AppConfig delays_;
  mutable std::fstream file_;
  bool delete_on_destroy_ = false;
  std::size_t position_ = 0;
  std::size_t length_ = 0;
};

[[nodiscard]] std::size_t int32_cells_in_file(std::filesystem::path const& path);

} // namespace tatlin
