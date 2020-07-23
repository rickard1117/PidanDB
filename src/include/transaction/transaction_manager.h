#pragma once

namespace pidan {
class Transaction;
/**
 * TransactionManager 维护负责创建、提交、终止和回滚事务，同时也负责维护所有全局事务的状态。
 */
class TransactionManager {
 public:
  // 开始一个事务
  Transaction *BeginTransaction();

  // 提交一个事务
  void Commit(Transaction *txn);

  // 终止一个事务，会回滚它做出的所有改动。
  void Abort(Transaction *txn);
};

}  // namespace pidan