#include "tatlin/file_tape_factory.hpp"

#include "tatlin/file_tape.hpp"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <utility>

namespace tatlin {

FileTapeFactory::FileTapeFactory(AppConfig config, std::filesystem::path tmp_dir)
    : config_(config)
    , tmp_dir_(std::move(tmp_dir)) {
  std::error_code ec;
  std::filesystem::create_directories(tmp_dir_, ec);
  if (ec) {
    throw TapeIOError("cannot create tmp directory: " + tmp_dir_.string() + ": " + ec.message());
  }
}

std::unique_ptr<Tape> FileTapeFactory::create_tape_from_file(std::filesystem::path const& path) {
  return std::make_unique<FileTape>(path, config_);
}

std::unique_ptr<Tape> FileTapeFactory::create_tape_by_path(std::filesystem::path const& path, std::size_t cell_count) {
  prepare_tape_file(path, cell_count);
  return std::make_unique<FileTape>(path, config_);
}

std::unique_ptr<Tape> FileTapeFactory::create_temporary_tape(std::size_t cell_count) {
  std::filesystem::path path = next_temporary_path();
  prepare_tape_file(path, cell_count);
  return std::make_unique<FileTape>(path, config_, true);
}

std::filesystem::path FileTapeFactory::next_temporary_path() {
  return tmp_dir_ / (std::string("tape_") + std::to_string(temporary_serial_++) + ".bin");
}

void FileTapeFactory::prepare_tape_file(std::filesystem::path const& path, std::size_t cell_count) {
  std::ofstream out(path, std::ios::binary | std::ios::trunc);
  if (!out) {
    throw TapeIOError("cannot create tape file: " + path.string());
  }

  if (cell_count == 0) {
    return;
  }

  out.seekp(static_cast<std::streamoff>((cell_count - 1) * sizeof(std::int32_t)), std::ios::beg);
  std::int32_t zero = 0;
  out.write(reinterpret_cast<char const*>(&zero), sizeof(zero));
  if (!out) {
    throw TapeIOError("cannot allocate tape file: " + path.string());
  }
}

} // namespace tatlin
