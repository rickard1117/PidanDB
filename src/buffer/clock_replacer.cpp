#include "buffer/clock_replacer.h"

#include <assert.h>

namespace pidan {

ClockReplacer::ClockReplacer() : clock_index_(0), size_(0) {}

void ClockReplacer::Unpin(frame_id_t frame_id) {
  auto it = pin_table_.find(frame_id);
  if (it == pin_table_.cend()) {
    clock_.insert(clock_.begin() + clock_index_, frame_id);
    pin_table_[frame_id] = NewFrame();
    clock_index_++;
    size_++;
    return;
  }
  if (IsPin(it->second)) {
    UnPin(it->second);
    AddRef(it->second);
    size_++;
  }
}

void ClockReplacer::Pin(frame_id_t frame_id) {
  auto it = pin_table_.find(frame_id);
  if (it == pin_table_.cend() || IsPin(it->second)) {
    return;
  }
  Pin(it->second);
  size_--;
}

bool ClockReplacer::Victim(frame_id_t *frame_id) {
  if (size_ == 0) {
    return false;
  }

  for (;;) {
    if (clock_index_ == clock_.size()) {
      clock_index_ = 0;
      continue;
    }
    auto frame = clock_[clock_index_];
    auto it = pin_table_.find(frame);
    assert(it != pin_table_.end());

    if (IsPin(it->second)) {
      clock_index_++;
      continue;
    }
    if (IsRef(it->second)) {
      DecRef(it->second);
      clock_index_++;
      continue;
    }
    *frame_id = frame;
    clock_.erase(clock_.begin() + clock_index_);
    auto n = pin_table_.erase(*frame_id);
    assert(n == 1);
    size_--;
    return true;
  }
}

}  // namespace pidan