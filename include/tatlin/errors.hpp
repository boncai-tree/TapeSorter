#pragma once

#include <stdexcept>

namespace tatlin {

class TatlinError : public std::runtime_error {
public:
  using std::runtime_error::runtime_error;
};

class TapeError : public TatlinError {
public:
  using TatlinError::TatlinError;
};

class TapeIOError : public TapeError {
public:
  using TapeError::TapeError;
};

class TapeBoundsError : public TapeError {
public:
  using TapeError::TapeError;
};

class InvariantError : public std::logic_error {
public:
  using std::logic_error::logic_error;
};

} // namespace tatlin
