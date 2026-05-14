#pragma once

#include "tape.hpp"

#include <cstddef>
#include <memory>

namespace tatlin {

class TapeFactory {
public:
  virtual ~TapeFactory() = default;

  [[nodiscard]] virtual std::unique_ptr<Tape> create_temporary_tape(std::size_t cell_count) = 0;
};

} // namespace tatlin
