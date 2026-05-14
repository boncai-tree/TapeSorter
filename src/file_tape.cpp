#include "tatlin/file_tape.hpp"

#include <cstdint>
#include <fstream>
#include <iostream>
#include <thread>
#include <utility>

namespace tatlin {

std::size_t int32_cells_in_file(std::filesystem::path const& path) {
  std::uintmax_t bytes = std::filesystem::file_size(path);
  if (bytes % sizeof(std::int32_t) != 0) {
    throw TapeError("tape file size is not a multiple of sizeof(int32_t): " + path.string());
  }
  return static_cast<std::size_t>(bytes / sizeof(std::int32_t));
}

FileTape::~FileTape() {
  if (file_.is_open()) {
    file_.flush();
    if (!file_) {
      std::cerr << "tape file flush failed (data may not be fully on disk): " << path_.string() << std::endl;
    }
    file_.close();
  }
  if (delete_on_destroy_) {
    std::error_code ec;
    std::filesystem::remove(path_, ec);
  }
}

FileTape::FileTape(std::filesystem::path path, AppConfig delays, bool delete_on_destroy)
    : path_(std::move(path))
    , delays_(delays)
    , delete_on_destroy_(delete_on_destroy) {
  file_.open(path_, std::ios::binary | std::ios::in | std::ios::out);
  if (!file_) {
    throw TapeIOError("cannot open tape file: " + path_.string());
  }

  length_ = int32_cells_in_file(path_);

  position_ = 0;
  seek_to_position();
}

void FileTape::apply_read_delay() const {
  if (delays_.read_delay.count() > 0) {
    std::this_thread::sleep_for(delays_.read_delay);
  }
}

void FileTape::apply_write_delay() const {
  if (delays_.write_delay.count() > 0) {
    std::this_thread::sleep_for(delays_.write_delay);
  }
}

void FileTape::apply_rewind_delay() const {
  if (delays_.rewind_delay.count() > 0) {
    std::this_thread::sleep_for(delays_.rewind_delay);
  }
}

void FileTape::apply_shift_delay() const {
  if (delays_.shift_delay.count() > 0) {
    std::this_thread::sleep_for(delays_.shift_delay);
  }
}

void FileTape::seek_to_position() const {
  std::streamoff off = static_cast<std::streamoff>(position_) * static_cast<std::streamoff>(sizeof(std::int32_t));
  file_.clear();
  file_.seekg(off, std::ios::beg);
  file_.seekp(off, std::ios::beg);
  if (!file_) {
    throw TapeIOError("seek failed on FileTape: " + path_.string());
  }
}

std::int32_t FileTape::read() {
  apply_read_delay();
  seek_to_position();
  std::int32_t value = 0;
  file_.read(reinterpret_cast<char*>(&value), sizeof(value));
  if (!file_ || file_.gcount() != static_cast<std::streamsize>(sizeof(value))) {
    throw TapeIOError("read failed on tape: " + path_.string());
  }
  return value;
}

void FileTape::write(std::int32_t value) {
  apply_write_delay();
  seek_to_position();
  file_.write(reinterpret_cast<char const*>(&value), sizeof(value));
  if (!file_) {
    throw TapeIOError("write failed on tape: " + path_.string());
  }
}

void FileTape::shift_next() {
  if (position_ >= length_) {
    throw TapeBoundsError("shift_next after end of tape: " + path_.string());
  }
  apply_shift_delay();
  ++position_;
}

void FileTape::shift_prev() {
  if (at_begin()) {
    throw TapeBoundsError("shift_prev before begin of tape: " + path_.string());
  }
  apply_shift_delay();
  --position_;
}

void FileTape::rewind() {
  apply_rewind_delay();
  position_ = 0;
}

bool FileTape::at_begin() const {
  return position_ == 0;
}

bool FileTape::at_end() const {
  return position_ == length_;
}

} // namespace tatlin
