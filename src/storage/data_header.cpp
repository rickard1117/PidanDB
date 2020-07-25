#include "storage/data_header.h"

#include <cassert>

#include "transaction/transaction.h"

namespace pidan {

bool DataHeader::Put(Transaction *txn, const Slice &val) {
  assert(txn->Type() == TransactionType::WRITE);
  if (!latch_.TryWriteLock(txn->ID())) {
    return false;
  }
  auto undo = txn->NewUndoRecordForPut(this, val);
  auto version_chain = version_chain_.load();
  // 这里不需要原子操作
  undo->Next() = version_chain;
  // 但是对version chain的修改需要原子操作，因为可能其他读事务正准备访问version chain
  auto result = version_chain_.compare_exchange_strong(version_chain, undo);
  // 这里一定不会失败，因为我们已经加了写锁
  assert(result == true);
  return true;
}

bool DataHeader::Select(Transaction *txn, std::string *val) {
  if (txn->Type() == TransactionType::READ) {
    // 读事务不用加锁
    UndoRecord *undo = version_chain_.load();
    // version chain上的DataEntry是按照时间戳从大到小排序的。
    // 我们要在version chain上找到第一个小于txn.ID()的UndoRecord
    while (undo->NewerThan(txn->ID())) {
      undo = undo->Next().load();
      // 这里一定不会读到version chain的末尾。
      // 因为有任何事务处于激活状态时，GC都会至少为它保留一个版本，除非是删除。
      // 删除情况下，GC可以直接从index中将整个data header删除。
      // 这时这里会得到一个删除类型的UndoRecord
      assert(undo != nullptr);
    }

    if (undo->Type() == UndoRecordType::DELETE) {
      return true;
    }

    undo->GetData(val);
    return true;
  }

  // 写事务要加读锁
  if (!latch_.TryReadLock()) {
    return false;
  }

  // 写事务的读操作直接读取最新内容。
  UndoRecord *undo = version_chain_.load();
  assert(undo != nullptr);
  txn->AddRead(undo);
  if (undo->Type() == UndoRecordType::DELETE) {
    return true;
  }
  undo->GetData(val);
  return true;
}

}  // namespace pidan