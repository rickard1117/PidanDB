
#include "transaction/transaction_manager.h"

#include <tbb/spin_mutex.h>

#include <cassert>

#include "storage/data_header.h"
#include "transaction/transaction.h"

namespace pidan {

Transaction *TransactionManager::BeginWriteTransaction() {
  return new Transaction(TransactionType::WRITE, ts_manager_->BeginTransaction());
}

Transaction TransactionManager::BeginReadTransaction() {
  return Transaction(TransactionType::READ, ts_manager_->BeginTransaction());
}

void TransactionManager::Commit(Transaction *txn) {
  if (txn->Type() == TransactionType::READ) {
    return;
  }

  if (txn->iso_lv_ == IsolationLevel::READ_COMMITTED) {
    // 这里不用加锁，因为在MakeWriteVisible执行之前没有其他事务能读到txn的修改
    // 但如果其他事务重复读的话，可能会读到不同的值。所以隔离级别是读已提交。
    timestamp_t now_ts = ts_manager_->CurrentTime();
    txn->MakeWriteVisible(now_ts + 1);
    ts_manager_->CheckOutTimestamp();
    txn->RealseAllReadLock();
    return;
  }

  assert(txn->iso_lv_ == IsolationLevel::SERIALIZABLE);
  {
    // 这里要加锁，因为同一时间只能有一个事务提交，不能有其他事务在此期间去修改全局的timestamp
    SpinLatch::ScopedSpinLatch lock(&commit_lock_);
    timestamp_t now_ts = ts_manager_->CurrentTime();
    txn->MakeWriteVisible(now_ts + 1);
    ts_manager_->CheckOutTimestamp();
  }
  txn->RealseAllReadLock();

  SpinLatch::ScopedSpinLatch lock(&completed_txn_lock_);
  completed_txn_.push_back(txn);
}

void TransactionManager::Abort(Transaction *txn) {
  // 回滚事务，就是要删除所有此事务创建的新版本。
  txn->Rollback();
}

}  // namespace pidan