#include "tatlin/tape_sorter.hpp"

#include "tatlin/app_config.hpp"

#include <queue>

#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <vector>

namespace tatlin {
namespace {

constexpr std::size_t kHardTemporaryTapeLimit = 128;

struct HeapNode {
  std::int32_t value = 0;
  std::size_t source = 0;

  bool operator>(HeapNode const& other) const noexcept {
    return value > other.value;
  }
};

struct RunSource {
  Tape* tape = nullptr;
  std::size_t length = 0;
};

struct TapeWithRuns {
  std::unique_ptr<Tape> tape;
  std::vector<std::size_t> run_lengths;
};

std::size_t total_run_count(std::vector<TapeWithRuns> const& bank) {
  std::size_t n = 0;
  for (auto const& slot : bank) {
    n += slot.run_lengths.size();
  }
  return n;
}

void merge_runs(std::vector<RunSource>& inputs, Tape& output) {
  std::priority_queue<HeapNode, std::vector<HeapNode>, std::greater<>> heap;
  std::vector<std::size_t> remaining(inputs.size());

  for (std::size_t i = 0; i < inputs.size(); ++i) {
    remaining[i] = inputs[i].length;
    if (remaining[i] == 0) {
      continue;
    }

    heap.push(HeapNode{inputs[i].tape->read(), i});
    inputs[i].tape->shift_next();
    --remaining[i];
  }

  while (!heap.empty()) {
    HeapNode top = heap.top();
    heap.pop();

    output.write(top.value);
    output.shift_next();

    if (remaining[top.source] == 0) {
      continue;
    }

    heap.push(HeapNode{inputs[top.source].tape->read(), top.source});
    inputs[top.source].tape->shift_next();
    --remaining[top.source];
  }
}

} // namespace

TapeSorter::TapeSorter(TapeFactory* tape_factory, std::size_t ints_capacity, std::size_t configured_temporary_tapes_k)
    : tape_factory_(tape_factory)
    , ints_capacity_(ints_capacity)
    , configured_temporary_tapes_k_(configured_temporary_tapes_k) {
  if (tape_factory_ == nullptr) {
    throw InvariantError("TapeSorter requires non-null tape factory");
  }
  if (ints_capacity_ < 2) {
    throw ConfigError("memory_limit_bytes allows fewer than two int32 values");
  }
}

void TapeSorter::sort(Tape& input, Tape& output, std::size_t total_elements) {
  if (total_elements == 0) {
    return;
  }

  std::size_t tape_count = std::min({configured_temporary_tapes_k_, ints_capacity_, kHardTemporaryTapeLimit});
  tape_count -= tape_count % 2;

  if (tape_count < 4) {
    throw ConfigError("Two-bank merge-sort requires at least 4 temp tapes");
  }

  std::size_t bank_size = tape_count / 2;
  std::size_t chunk_size = ints_capacity_;

  std::vector<TapeWithRuns> banks[2];

  for (auto& bank : banks) {
    bank.resize(bank_size);
    for (auto& slot : bank) {
      slot.tape = tape_factory_->create_temporary_tape(total_elements);
    }
  }

  std::size_t active_bank = 0;
  std::size_t write_slot = 0;

  std::size_t elements_remaining = total_elements;
  while (elements_remaining > 0) {
    std::vector<std::int32_t> chunk;
    chunk.reserve(chunk_size);

    while (chunk.size() < chunk_size && elements_remaining > 0) {
      chunk.push_back(input.read());
      input.shift_next();
      --elements_remaining;
    }

    std::ranges::sort(chunk);

    auto& slot = banks[active_bank][write_slot];
    for (std::int32_t value : chunk) {
      slot.tape->write(value);
      slot.tape->shift_next();
    }
    slot.run_lengths.push_back(chunk.size());

    write_slot = (write_slot + 1) % bank_size;
  }

  while (total_run_count(banks[active_bank]) > 1) {
    std::size_t next_bank = active_bank ^ 1;

    for (auto& slot : banks[next_bank]) {
      slot.run_lengths.clear();
      slot.tape->rewind();
    }

    for (auto& slot : banks[active_bank]) {
      slot.tape->rewind();
    }

    for (std::size_t level = 0;; ++level) {
      std::vector<RunSource> batch;
      batch.reserve(bank_size);

      for (std::size_t tape = 0; tape < bank_size; ++tape) {
        if (level < banks[active_bank][tape].run_lengths.size()) {
          batch.push_back({banks[active_bank][tape].tape.get(), banks[active_bank][tape].run_lengths[level]});
        }
      }

      if (batch.empty()) {
        break;
      }

      std::size_t merged_length = 0;
      for (auto& run : batch) {
        merged_length += run.length;
      }

      std::size_t dest_tape = level % bank_size;
      merge_runs(batch, *banks[next_bank][dest_tape].tape);
      banks[next_bank][dest_tape].run_lengths.push_back(merged_length);
    }

    active_bank = next_bank;
  }

  for (auto& slot : banks[active_bank]) {
    if (slot.run_lengths.empty()) {
      continue;
    }

    slot.tape->rewind();
    std::size_t length = slot.run_lengths.front();
    for (std::size_t i = 0; i < length; ++i) {
      output.write(slot.tape->read());
      output.shift_next();
      slot.tape->shift_next();
    }
    return;
  }
}

} // namespace tatlin
