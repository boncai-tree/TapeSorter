#pragma once

#include "errors.hpp"

#include <chrono>
#include <cstddef>
#include <filesystem>

namespace tatlin {

struct AppConfig {
  std::chrono::milliseconds read_delay = std::chrono::milliseconds(0);
  std::chrono::milliseconds write_delay = std::chrono::milliseconds(0);
  std::chrono::milliseconds rewind_delay = std::chrono::milliseconds(0);
  std::chrono::milliseconds shift_delay = std::chrono::milliseconds(0);
  std::size_t memory_limit_bytes = 16 * 1024 * 1024;
  std::size_t temporary_tapes_k = 32;
};

[[nodiscard]] AppConfig load_config(std::filesystem::path const& path);

class ConfigError : public TatlinError {
public:
  using TatlinError::TatlinError;
};

} // namespace tatlin
