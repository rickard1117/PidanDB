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

  TimestampManager() = default;

  timestamp_t CheckOutTimestamp() { return timestamp_.fetch_add(1); }

  timestamp_t CurrentTime() const { return timestamp_.load(); }

 private:
  std::atomic<timestamp_t> timestamp_{INIT_TIMESTAMP};
};

}  // namespace pidan