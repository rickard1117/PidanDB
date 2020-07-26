#pragma once
#include <mutex>

#include "transaction/timestamp_manager.h"
namespace pidan {
class Transaction;
/**
 * TransactionManager 维护负责创建、提交、终止和回滚事务，同时也负责维护所有全局事务的状态。
 */
class TransactionManager {
 public:
  DISALLOW_COPY_AND_MOVE(TransactionManager);

  TransactionManager(TimestampManager *ts_manager) : ts_manager_(ts_manager) {}

  // 开始一个写事务
  Transaction *BeginWriteTransaction();

  // 开始一个读事务
  Transaction *BeginReadTransaction();

  // 提交一个事务
  void Commit(Transaction *txn);

  // 终止一个事务，会回滚它做出的所有改动。
  void Abort(Transaction *txn);

 private:
  TimestampManager *ts_manager_;
  txn_id_t txn_auto_id_{INIT_TXN_ID};
  std::mutex commit_lock_;  // 此锁保证同一时间只能有一个事务进行提交
};

}  // namespace pidan