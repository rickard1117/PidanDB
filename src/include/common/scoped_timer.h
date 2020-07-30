#pragma once

#include <chrono>

#include "macros.h"

namespace pidan {

template <class resolution>
class ScopedTimer {
 public:
  DISALLOW_COPY_AND_MOVE(ScopedTimer);
  explicit ScopedTimer(uint64_t *const elapsed)
      : start_(std::chrono::high_resolution_clock::now()), elapsed_(elapsed) {}

  ~ScopedTimer() {
    auto end = std::chrono::high_resolution_clock::now();
    *elapsed_ = static_cast<uint64_t>(std::chrono::duration_cast<resolution>(end - start_).count());
  }

 private:
  const std::chrono::high_resolution_clock::time_point start_;
  uint64_t *const elapsed_;
};

}  // namespace pidan
