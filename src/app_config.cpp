#include "tatlin/app_config.hpp"

#include <cctype>
#include <charconv>
#include <chrono>
#include <concepts>
#include <cstdint>
#include <fstream>
#include <string>

namespace tatlin {
namespace {

void trim_inplace(std::string& s) {
  std::size_t first = 0;
  while (first < s.size() && std::isspace(static_cast<unsigned char>(s[first]))) {
    ++first;
  }

  std::size_t last = s.size();
  while (last > first && std::isspace(static_cast<unsigned char>(s[last - 1]))) {
    --last;
  }

  s.erase(last);
  s.erase(0, first);
}

template <std::integral IntType>
[[nodiscard]] IntType parse_integral(std::string const& value, char const* context) {
  IntType out = 0;
  char const* begin = value.data();
  char const* end = begin + value.size();
  std::from_chars_result const res = std::from_chars(begin, end, out);
  if (res.ptr != end || res.ec != std::errc{}) {
    throw ConfigError(std::string("invalid integer (") + context + ")");
  }
  return out;
}

[[nodiscard]] std::chrono::milliseconds parse_delay_milliseconds(std::string const& value) {
  auto ms = parse_integral<long long>(value, "delay_ms");
  if (ms < 0) {
    throw ConfigError("delay must be non-negative");
  }
  return std::chrono::milliseconds(ms);
}

[[nodiscard]] std::size_t parse_memory_limit_bytes(std::string const& value) {
  auto n = parse_integral<std::size_t>(value, "memory_limit_bytes");
  if (n == 0) {
    throw ConfigError("memory_limit_bytes must be > 0");
  }
  return n;
}

[[nodiscard]] std::size_t parse_temporary_tapes_k(std::string const& value) {
  auto k = parse_integral<std::size_t>(value, "temporary_tapes_k");
  if (k < 2) {
    throw ConfigError("temporary_tapes_k must be >= 2");
  }
  return k;
}

} // namespace

AppConfig load_config(std::filesystem::path const& path) {
  std::ifstream in(path);
  if (!in) {
    throw ConfigError("cannot open config file: " + path.string());
  }

  AppConfig cfg;
  std::string line;
  while (std::getline(in, line)) {
    trim_inplace(line);

    if (line.empty() || line.front() == '#') {
      continue;
    }

    std::size_t const eq = line.find('=');
    if (eq == std::string::npos) {
      throw ConfigError("expected key=value: " + line);
    }

    std::string key = line.substr(0, eq);
    std::string value = line.substr(eq + 1);
    trim_inplace(key);
    trim_inplace(value);

    if (key == "read_delay_ms") {
      cfg.read_delay = parse_delay_milliseconds(value);
    } else if (key == "write_delay_ms") {
      cfg.write_delay = parse_delay_milliseconds(value);
    } else if (key == "rewind_delay_ms") {
      cfg.rewind_delay = parse_delay_milliseconds(value);
    } else if (key == "shift_delay_ms") {
      cfg.shift_delay = parse_delay_milliseconds(value);
    } else if (key == "memory_limit_bytes") {
      cfg.memory_limit_bytes = parse_memory_limit_bytes(value);
    } else if (key == "temporary_tapes_k") {
      cfg.temporary_tapes_k = parse_temporary_tapes_k(value);
    }
  }

  if (cfg.memory_limit_bytes / sizeof(std::int32_t) < 2) {
    throw ConfigError("The memory_limit_bytes parameter allows for less than two int32 values");
  }

  return cfg;
}

} // namespace tatlin
