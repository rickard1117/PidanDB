#pragma once

#include <atomic>

#include "common/macros.h"
#include "common/type.h"

namespace pidan {

// 用于数据项的锁。为写事务提供读写锁（只读事务不需要加锁）。
// 读写锁之间互相冲突，first writer win。读锁之间不冲突。
// 同一个事务如果已经加了写锁，再次加读锁不会失败，解锁时只需要解除写锁即可。
class DataLatch {
 public:
  DISALLOW_COPY_AND_MOVE(DataLatch);

  DataLatch() = default;

  // 尝试为此数据项加写锁，加锁成功返回true，否则返回false
  bool TryWriteLock(txn_id_t txn_id);

  // 尝试为此数据项加读锁，加锁成功返回true，否则返回false
  bool TryReadLock(txn_id_t txn_id);

  void ReadUnlock();

  void WriteUnlock(txn_id_t txn_id);

 private:
  // 根据latch_flag的值返回一个新值，新的值表示加了读锁。
  uint64_t SetReadBits(uint64_t latch) { return latch + (1UL << 48); }
  // 
  uint64_t UnsetReadBits(uint64_t latch) { return latch - (1UL << 48); }

  // 判断flag上是否已经加了写锁了
  bool LockedOnWrite(uint64_t latch) {
    return GetWriteBits(latch) != NULL_DATA_LATCH;
  }
  //
  bool LockedOnRead(uint64_t latch) {
    return GetReadBits(latch) != NULL_DATA_LATCH;
  }
  
  uint64_t GetReadBits(uint64_t latch) {
    return latch & ~((1UL << 48) - 1);
  }

  uint64_t GetWriteBits(uint64_t latch) {
    return latch & ((1UL << 48) - 1);
  }

  std::atomic<uint64_t> latch_{NULL_DATA_LATCH};
};



}  // namespace pidan
