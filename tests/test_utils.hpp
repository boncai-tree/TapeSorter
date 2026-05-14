#pragma once

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

namespace tatlin::test {

struct TempDir {
  std::filesystem::path path;

  TempDir() {
    static std::mt19937_64 rng{std::random_device{}()};
    path = std::filesystem::temp_directory_path() / ("yadro_tatlin_test_" + std::to_string(rng()));
    std::filesystem::create_directories(path);
  }

  ~TempDir() {
    std::error_code ec;
    std::filesystem::remove_all(path, ec);
  }

  TempDir(TempDir const&) = delete;
  TempDir& operator=(TempDir const&) = delete;
};

inline void write_int32_file(std::filesystem::path const& p, std::vector<std::int32_t> const& data) {
  std::ofstream out(p, std::ios::binary | std::ios::trunc);
  if (!out) {
    throw std::runtime_error("write_int32_file: cannot open " + p.string());
  }
  for (std::int32_t v : data) {
    out.write(reinterpret_cast<char const*>(&v), sizeof(v));
  }
  if (!out) {
    throw std::runtime_error("write_int32_file: write failed " + p.string());
  }
}

inline std::vector<std::int32_t> read_int32_file(std::filesystem::path const& p) {
  std::ifstream in(p, std::ios::binary);
  if (!in) {
    throw std::runtime_error("read_int32_file: cannot open " + p.string());
  }
  std::vector<std::int32_t> out;
  std::int32_t v = 0;
  while (in.read(reinterpret_cast<char*>(&v), sizeof(v))) {
    if (in.gcount() != static_cast<std::streamsize>(sizeof(v))) {
      throw std::runtime_error("read_int32_file: partial read " + p.string());
    }
    out.push_back(v);
  }
  return out;
}

} // namespace tatlin::test
