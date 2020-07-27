#include "common/nowait_rw_latch.h"

namespace pidan {

bool NoWaitRWLatch::TryWriteLock() {
    uint64_t null_latch = NULL_DATA_LATCH;
    return latch_.compare_exchange_strong(null_latch, write_latch_status); 
}

bool NoWaitRWLatch::TryReadLock() {
  while (true) {
    auto latch = latch_.load();
    if (LockedOnWrite(latch)) {
      return false;
    }
    if (!latch_.compare_exchange_strong(latch, latch + 1)) {
      continue;
    }
    return true;
  }
}

void NoWaitRWLatch::WriteUnlock() { latch_.store(NULL_DATA_LATCH); }

void NoWaitRWLatch::ReadUnlock() { latch_.fetch_sub(1); }

bool NoWaitRWLatch::UpgradeToWriteLock() {
  while (true) {
    auto latch = latch_.load();
    if (latch == 1UL) {
      // 证明当前latch上只有一个读者（就是调用者自己）
      if (!latch_.compare_exchange_strong(latch, write_latch_status)) {
        continue;
      }
      return true;
    }
    return false;
  }
}

}  // namespace pidan