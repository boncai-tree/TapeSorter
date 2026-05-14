#include "tatlin/app_config.hpp"
#include "tatlin/file_tape.hpp"
#include "tatlin/file_tape_factory.hpp"
#include "tatlin/tape_sorter.hpp"

#include <cstdint>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>

namespace {

[[nodiscard]] std::filesystem::path default_config_path() {
  return std::filesystem::current_path() / "tape_config.ini";
}

[[nodiscard]] tatlin::AppConfig load_or_defaults(std::filesystem::path const* config_path) {
  if (config_path == nullptr) {
    return tatlin::AppConfig{};
  }

  try {
    return tatlin::load_config(*config_path);
  } catch (tatlin::ConfigError const& e) {
    std::cerr << "configuration error (" << config_path->string() << "): " << e.what() << std::endl;
    throw;
  } catch (...) {
    std::cerr << "failed to load config: " << config_path->string() << std::endl;
    throw;
  }
}

}  // namespace

int main(int argc, char* argv[]) {
  try {
    if (argc != 3 && argc != 4) {
      std::cerr << "usage: " << argv[0] << " <input.bin> <output.bin> [config.ini]" << std::endl;
      return 1;
    }

    std::filesystem::path input_path = argv[1];
    std::filesystem::path output_path = argv[2];
    bool has_explicit_config = argc == 4;
    std::filesystem::path cfg_storage;
    std::filesystem::path* cfg_ptr = nullptr;

    tatlin::AppConfig cfg;
    if (has_explicit_config) {
      cfg_storage = argv[3];
      cfg_ptr = &cfg_storage;
      cfg = load_or_defaults(cfg_ptr);
    } else {
      std::filesystem::path guess = default_config_path();
      if (std::filesystem::exists(guess)) {
        cfg_storage = guess;
        cfg_ptr = &cfg_storage;
        cfg = load_or_defaults(cfg_ptr);
      } else {
        cfg = load_or_defaults(nullptr);
      }
    }


    tatlin::FileTapeFactory tape_factory(cfg, std::filesystem::current_path() / "tmp");
    std::size_t input_length = tatlin::int32_cells_in_file(input_path);
    std::unique_ptr<tatlin::Tape> input_tape = tape_factory.create_tape_from_file(input_path);
    std::unique_ptr<tatlin::Tape> output_tape = tape_factory.create_tape_by_path(output_path, input_length);
    std::size_t ints_capacity = cfg.memory_limit_bytes / sizeof(std::int32_t);

    tatlin::TapeSorter sorter(&tape_factory, ints_capacity, cfg.temporary_tapes_k);
    sorter.sort(*input_tape, *output_tape, input_length);

    return 0;
  } catch (std::exception const& e) {
    std::cerr << "error: " << e.what() << std::endl;
    return 1;
  }
}

