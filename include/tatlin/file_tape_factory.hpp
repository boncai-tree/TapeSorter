#pragma once

#include "app_config.hpp"
#include "tape_factory.hpp"

#include <filesystem>

namespace tatlin {

class FileTapeFactory final : public TapeFactory {
public:
  FileTapeFactory(AppConfig config, std::filesystem::path tmp_dir);

  [[nodiscard]] std::unique_ptr<Tape> create_tape_from_file(std::filesystem::path const& path);

  [[nodiscard]] std::unique_ptr<Tape> create_tape_by_path(std::filesystem::path const& path, std::size_t cell_count);

  [[nodiscard]] std::unique_ptr<Tape> create_temporary_tape(std::size_t cell_count) override;

private:
  void prepare_tape_file(std::filesystem::path const& path, std::size_t cell_count);
  [[nodiscard]] std::filesystem::path next_temporary_path();

  AppConfig config_;
  std::filesystem::path tmp_dir_;
  std::size_t temporary_serial_ = 0;
};

} // namespace tatlin
