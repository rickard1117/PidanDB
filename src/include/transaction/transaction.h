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

  Transaction(TransactionType type, timestamp_t t, uint64_t id) : type_(type), timestamp_(t), id_(id) {}

  timestamp_t ID() const { return id_; }

  UndoRecord *NewUndoRecordForPut(DataHeader *data_header, const Slice &val);

  TransactionType Type() const { return type_; }

  void AddRead(UndoRecord *undo) { read_set_.push_back(undo); }

  void AddWrite(UndoRecord *undo) { write_set_.insert(undo); }

  bool LockedOnWrite(UndoRecord *undo) { return write_set_.find(undo) != write_set_.cend(); }

 private:
  friend class TransactionManager;
  // 针对写事务的写集合和读集合，用于在事务提交或终止后释放锁、GC等操作。
  // write_set中不可以有重复元素，因为写锁只能释放一次。
  std::set<UndoRecord *> write_set_;
  std::vector<UndoRecord *> read_set_;
  TransactionType type_;
  timestamp_t timestamp_;  // 表示事务开始的时间戳，不同事务可能开始于同一个时间戳
  uint64_t id_;            // 每个事务唯一的id
};

}  // namespace pidan