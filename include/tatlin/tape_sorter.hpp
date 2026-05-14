#pragma once

#include "tape.hpp"
#include "tape_factory.hpp"

namespace tatlin {

class TapeSorter {
public:
  TapeSorter(TapeFactory* tape_factory, std::size_t ints_capacity, std::size_t configured_temporary_tapes_k);

  void sort(Tape& input, Tape& output, std::size_t total_elements);

private:
  TapeFactory* tape_factory_ = nullptr;
  std::size_t ints_capacity_ = 0;
  std::size_t configured_temporary_tapes_k_ = 0;
};

} // namespace tatlin
