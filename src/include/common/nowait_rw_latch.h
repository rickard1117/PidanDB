#pragma once

#include <atomic>

#include "common/macros.h"
#include "common/type.h"

namespace pidan {

// 一个不支持等待的非递归读写锁
// 读写锁之间互相冲突，写锁之间互相冲突，读锁之间不冲突。
// 锁是不可递归的，不能对同一个对象重复加锁（不管是否是同一个类型的锁）。
class NoWaitRWLatch {
 public:
  DISALLOW_COPY_AND_MOVE(NoWaitRWLatch);

  NoWaitRWLatch() = default;

  // 尝试加写锁，加锁成功返回true，否则返回false
  bool TryWriteLock();

  // 尝试加读锁，加锁成功返回true，否则返回false
  bool TryReadLock();

  void ReadUnlock();

  void WriteUnlock();

  // 将读锁升级为写锁。当且仅当加读锁成功之后才可以调用此函数。
  bool UpgradeToWriteLock();

  bool NoLock() { return latch_.load() == NULL_DATA_LATCH; }

 private:
  bool LockedOnWrite(uint64_t latch) { return latch == write_latch_status; }

  // 加了写锁的状态
  static constexpr uint64_t write_latch_status = (1UL << 63);
  // 用最高位来表示是否加了写锁。
  // 用其他位表示读锁的计数。
  std::atomic<uint64_t> latch_{NULL_DATA_LATCH};
};

}  // namespace pidan