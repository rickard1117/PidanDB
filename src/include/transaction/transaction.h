#pragma once
#include <set>
#include <vector>

#include "common/slice.h"
#include "common/type.h"
#include "transaction/undo_record.h"

namespace pidan {

class DataHeader;

enum class TransactionType { WRITE, READ };

class Transaction {
 public:
  DISALLOW_COPY_AND_MOVE(Transaction);

  Transaction(TransactionType type, timestamp_t t) : type_(type), timestamp_(t) {}

  timestamp_t Timestamp() const {return timestamp_;}

  UndoRecord *NewUndoRecordForPut(DataHeader *data_header, const Slice &val);

  TransactionType Type() const { return type_; }

  void ReadLockOn(DataHeader *data_header);

  void WriteLockOn(DataHeader *data_header);

  bool AlreadyWriteLockOn(DataHeader *data_header);

  bool AlreadyReadLockOn(DataHeader *data_header);

  void UpgradeToWriteLock(DataHeader *data_header);

 private:
  friend class TransactionManager;
  // 令写操作可见，用于事务提交。
  void MakeWriteVisible(timestamp_t timestamp);

  // 释放所有读锁，用于事务提交。
  void RealseAllReadLock();

  void RelaseAllWriteLock();

  std::vector<UndoRecord *> write_set_;  // 所有由此事务创建的UndoRecord集合,这里一定不会有重复元素
  std::set<DataHeader *> write_lock_set_;  // 加了写锁的DataHeader集合
  // 加了读锁的DataHeader集合。如果一个DataHeader存在于write_lock_set_中，那它必须不能存在于read_lock_set_
  // read_lock_set_ 可以有重复元素因为同一个事务可以对同一个DataHeader重复加读锁
  std::set<DataHeader *> read_lock_set_;
  TransactionType type_;
  timestamp_t timestamp_;  // 表示事务开始的时间戳，不同事务可能开始于同一个时间戳
  // uint64_t id_;            // 每个事务唯一的id
};

}  // namespace pidan