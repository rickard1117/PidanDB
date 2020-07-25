#pragma once

#include <string>

#include "common/macros.h"
#include "common/slice.h"
#include "common/type.h"
#include "storage/data_entry.h"
#include "storage/data_latch.h"
#include "transaction/transaction.h"

namespace pidan {

// DataHeader 中保存了一条数据所有的多版本信息。通过B+树索引。
// DataHeader 不区分insert和update，这两种操作统一为put。
class DataHeader {
 public:
  DISALLOW_COPY_AND_MOVE(DataHeader);

  DataHeader() = default;

  // 插入一个新的值，插入成功返回true，否则返回false
  bool Put(Transaction *txn, const Slice &val);

  // 查找到对当前事务可见的值，成功返回true，否则返回false
  // 读事务一定不会失败。如果找不到，则不会对val做任何填充操作
  bool Select(Transaction *txn, std::string *val);

 private:
  friend class TransactionManager;
  // txn_id_表示了当前正在修改此数据项的事务，可以当做此数据项的latch_
  DataLatch latch_;
  std::atomic<UndoRecord *> version_chain_{nullptr};
};

}  // namespace pidan