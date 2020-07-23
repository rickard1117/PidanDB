#pragma once
#include <atomic>
#include <cstdint>

#include "common/macros.h"
#include "common/type.h"

namespace pidan {

/**
 * TimestampManager 负责对事务中需要的时间戳进行分配。
 */
class TimestampManager {
 public:
  DISALLOW_COPY_AND_MOVE(TimestampManager);

  TimestampManager() : timestamp_(0){};

  timestamp_t CheckOutTimestamp() { return time_++; }

  timestamp_t CurrentTime() const { return time_.load(); }

 private:
  std::atomic<timestamp_t> timestamp_;
};

}  // namespace pidan