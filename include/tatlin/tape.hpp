#pragma once

#include <cstdint>

namespace tatlin {

class Tape {
public:
  virtual ~Tape() = default;

  [[nodiscard]] virtual std::int32_t read() = 0;
  virtual void write(std::int32_t value) = 0;

  virtual void shift_next() = 0;
  virtual void shift_prev() = 0;
  virtual void rewind() = 0;

  [[nodiscard]] virtual bool at_begin() const = 0;
  [[nodiscard]] virtual bool at_end() const = 0;
};

} // namespace tatlin
