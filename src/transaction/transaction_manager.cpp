
#include "transaction/transaction_manager.h"

#include <tbb/spin_mutex.h>

#include <cassert>

#include "storage/data_header.h"
#include "transaction/transaction.h"

namespace pidan {

Transaction TransactionManager::BeginWriteTransaction() {
  return Transaction(TransactionType::WRITE, ts_manager_->CurrentTime());
}

Transaction TransactionManager::BeginReadTransaction() {
  return Transaction(TransactionType::READ, ts_manager_->CurrentTime());
}

void TransactionManager::Commit(Transaction *txn) {
  if (txn->Type() == TransactionType::READ) {
    return;
  }

  // 这里要加锁，因为同一时间只能有一个事务提交，不能有其他事务在此期间去修改全局的timestamp
  // const std::lock_guard<std::mutex> lock(commit_lock_);
  timestamp_t now_ts = ts_manager_->CheckOutTimestamp();
  // {
  //   // SpinLatch::ScopedSpinLatch lock(&commit_lock_);
  //   // timestamp_t now_ts = ts_manager_->CurrentTime();
    
  //   now_ts = ts_manager_->CheckOutTimestamp();
  // }
  txn->MakeWriteVisible(now_ts + 1);
  txn->RealseAllReadLock();
}

void TransactionManager::Abort(Transaction *txn) {
  // 回滚事务，就是要删除所有此事务创建的新版本。
  txn->Rollback();
}

}  // namespace pidan