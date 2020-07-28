#include "storage/data_header.h"

#include <cassert>

#include "transaction/transaction.h"

namespace pidan {

DataHeader::DataHeader(Transaction *txn) {
  auto result = latch_.TryWriteLock();
  assert(result);
  txn->WriteLockOn(this);
}

bool DataHeader::Put(Transaction *txn, const Slice &val) {
  assert(txn->Type() == TransactionType::WRITE);

  // txn如果没加写锁。分两种情况：1.已经加了读锁，那么尝试升级为写锁。
  // 2. 读锁也没加，那么尝试直接加写锁。
  if (!txn->AlreadyWriteLockOn(this)) {
    if (txn->AlreadyReadLockOn(this)) {
      if (!latch_.UpgradeToWriteLock()) {
        return false;
      }
      // 升级写锁成功
      txn->UpgradeToWriteLock(this);
    } else {
      if (!latch_.TryWriteLock()) {
        return false;
      }
      // 加写锁成功
      txn->WriteLockOn(this);
    }
  }

  auto undo = txn->NewUndoRecordForPut(this, val);
  auto version_chain = version_chain_.load();
  // 这里不需要原子操作，因为没有其他线程知道undo的存在。
  undo->Next() = version_chain;
  // 但是对version chain的修改需要原子操作，因为可能其他读事务正准备访问version chain
  auto result = version_chain_.compare_exchange_strong(version_chain, undo);
  // 这里一定不会失败，因为我们已经加了写锁
  assert(result == true);
  return true;
}

bool DataHeader::Select(Transaction *txn, std::string *val, bool *not_found) {
  if (txn->Type() == TransactionType::READ) {
    // 读事务不用加锁
    UndoRecord *undo = version_chain_.load();
    if (undo == nullptr) {
      *not_found = true;
      return true;
    }
    // version chain上的版本是按照时间戳从大到小（由新到旧）排序的。
    // 我们要在version chain上找到第一个小于txn.TS的UndoRecord
    while (undo->NewerThan(txn->Timestamp())) {
      undo = undo->Next().load();

      // 这里可能会读不到合适的版本。比如与读事务同时有一个写事务，创建了这个DataHeader
      // 但是还未提交，此时DataHeader里面所有的版本对读事务都是不可见的。
      // 对于读事务来说，这条数据并不存在
      if (undo == nullptr) {
        *not_found = true;
        return true;
      }
    }

    if (undo->Type() == UndoRecordType::DELETE) {
      *not_found = true;
      return true;
    }

    undo->GetData(val);
    *not_found = false;
    return true;
  }

  // 写事务要加读锁
  if (!txn->AlreadyWriteLockOn(this) && !txn->AlreadyReadLockOn(this)) {
    if (!latch_.TryReadLock()) {
      return false;
    }
    txn->ReadLockOn(this);
  }

  // 写事务的读操作直接读取最新内容。
  UndoRecord *undo = version_chain_.load();
  if (undo == nullptr) {
    *not_found = true;
    return true;
  }

  undo->GetData(val);
  *not_found = false;
  return true;
}

}  // namespace pidan