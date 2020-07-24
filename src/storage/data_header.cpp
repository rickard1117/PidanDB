#include "storage/data_header.h"

#include <cassert>

#include "transaction/transaction.h"

namespace pidan {

bool DataHeader::Put(Transaction *txn, const Slice &val) {
  assert(txn->Type() == TransactionType::WRITE);
  if (!latch_.TryWriteLock(txn->ID())) {
    return false;
  }
  auto undo = txn->NewUndoRecordForInsert(this, val);
  undo->Next()
  auto result = version_chain_.compare_exchange_strong();
  assert(result == true);
}

bool DataHeader::Select(Transaction *txn, std::string *val) {
  if (txn->Type() == TransactionType::READ) {
    // 读事务不用加锁
    DataEntry *data = version_chain_.load();
    // version chain上的DataEntry是按照时间戳从大到小排序的。
    // 我们要在version chain上找到第一个小于txn.ID()的DataEntry
    while (data->NewerThan(txn)) {
      data = data->Next();
      assert(data != null);
    }
    data->GetData(val);
    return true;
  }

  // 写事务要加读锁
  if (!latch_.TryReadLock()) {
    return false;
  }

  DataEntry *data = version_chain_.load();
  data->GetData(val);
  return true;
}

}  // namespace pidan