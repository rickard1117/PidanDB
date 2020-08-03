#pragma once
// #include <tbb/concurrent_hash_map.h>
#include <array>
#include <atomic>
#include <cstdint>

#include "common/config.h"
#include "common/macros.h"
#include "common/type.h"

namespace pidan {

/**
 * TimestampManager 负责对事务中需要的时间戳进行分配。
 */
class TimestampManager {
 public:
  DISALLOW_COPY_AND_MOVE(TimestampManager);

  TimestampManager() {
    for (int i = 0; i < MAX_ACCESS_THREAD; i++) {
      active_txns_[i] = MAX_TIMESTAMP;
    }
  };

  timestamp_t CheckOutTimestamp() { return timestamp_.fetch_add(1); }

  timestamp_t CurrentTime() const { return timestamp_.load(); }

  timestamp_t BeginTransaction();

  void EndTransaction();

  // 返回当前全局最老的事务开始的时间戳
  timestamp_t OldestTimestamp();

  // 只用于测试
  int ThreadID();

 private:
  std::atomic<timestamp_t> timestamp_{INIT_TIMESTAMP};
  std::atomic<timestamp_t> active_txns_[MAX_ACCESS_THREAD];  // 保存了每个线程里面
};

}  // namespace pidan