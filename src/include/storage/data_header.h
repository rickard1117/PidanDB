#pragma once

#include <string>

#include "common/macros.h"
#include "common/nowait_rw_latch.h"
#include "common/type.h"
#include "pidan/slice.h"
#include "storage/data_entry.h"
#include "transaction/transaction.h"

namespace pidan {

// DataHeader 中保存了一条数据所有的多版本信息。通过B+树索引。
// DataHeader 不区分insert和update，这两种操作统一为put。
class DataHeader {
 public:
  DISALLOW_COPY_AND_MOVE(DataHeader);

  DataHeader() = default;

  // 创建DataHeader时候，用txn对DataHeader加写锁
  DataHeader(Transaction *txn);

  // 插入一个新的值，插入成功返回true，否则返回false
  bool Put(Transaction *txn, const Slice &val);

  // 查找到对当前事务可见的值，成功返回true，否则返回false
  // 读事务一定不会失败。如果找不到，则不会对val做任何改动，并且not_found为true
  bool Select(Transaction *txn, std::string *val, bool *not_found);

  // 将此DataHeader标记为删除，只能由GC线程调用。
  // GC线程标记为删除后，会在将来某个GC周期将其占有的内存全部释放。
  void SetDelete() { to_be_deleted_.store(1); }

  bool IsDelete() { return to_be_deleted_.load() == 1; }

 private:
  friend class Transaction;
  NoWaitRWLatch latch_;
  std::atomic<uint32_t> to_be_deleted_{0};
  std::atomic<UndoRecord *> version_chain_{nullptr};
};

}  // namespace pidan