
#include "transaction/transaction_manager.h"

#include <cassert>

#include "storage/data_header.h"
#include "transaction/transaction.h"

namespace pidan {

Transaction *TransactionManager::BeginWriteTransaction() {
  return new Transaction(TransactionType::WRITE, ts_manager_->CurrentTime(), txn_auto_id_++);
}

Transaction *TransactionManager::BeginReadTransaction() {
  return new Transaction(TransactionType::READ, ts_manager_->CurrentTime(), txn_auto_id_++);
}

void TransactionManager::Commit(Transaction *txn) {
  if (txn->Type() == TransactionType::READ) {
    return;
  }

  // 这里要加锁，因为同一时间只能有一个事务提交，不能有其他事务在此期间去修改全局的timestamp
  const std::lock_guard<std::mutex> lock(commit_lock_);
  timestamp_t new_tx = ts_manager_->CurrentTime();
  for (auto &undo : txn->write_set_) {
    undo->SetTimestamp(new_tx + 1);
    undo->GetDataHeader()->latch_.WriteUnlock(txn->ID());
  }

  for (auto &undo : txn->read_set_) {
    undo->GetDataHeader()->latch_.ReadUnlock();
  }
  ts_manager_->CheckOutTimestamp();
}

void TransactionManager::Abort(Transaction *txn) { (void)txn; }

}  // namespace pidan