#include "storage/data_latch.h"

#include <cassert>

namespace pidan {

bool DataLatch::TryWriteLock(txn_id_t txn_id) {
  // 先检查此事务是不是已经对该数据项加过写锁了
  uint64_t flag = latch_.load();
  if (flag == txn_id) {
    return true;
  }

  if (flag != NULL_DATA_LATCH) {
    // 已经被加了某种形式的锁
    return false;
  }

  return latch_.compare_exchange_strong(flag, txn_id);
}

void DataLatch::WriteUnlock(txn_id_t txn_id) {
  auto result = latch_.compare_exchange_strong(txn_id, NULL_DATA_LATCH);
  assert(result);
}

bool DataLatch::TryReadLock(txn_id_t txn_id) {
  while (true) {
    uint64_t flag = latch_.load();
    if (flag == NULL_DATA_LATCH || LockedOnRead(flag)) {
      if (latch_.compare_exchange_strong(flag, SetReadBits(flag))) {
        return true;
      }
      continue;
    }

    assert(LockedOnWrite(flag));

    if (txn_id == GetWriteBits(flag)) {
      // 此事务已经加过写锁，不需要做任何操作。
      return true;
    }
    return false;
  }
}

void DataLatch::ReadUnlock() {
  while (true) {
    uint64_t flag = latch_.load();
    if (latch_.compare_exchange_strong(flag, UnsetReadBits(flag))) {
      return;
    }
    continue;
  }
}

}  // namespace pidan