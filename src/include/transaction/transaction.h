#pragma once
#include <set>

#include "common/slice.h"
#include "common/type.h"
#include "transaction/undo_record.h"

namespace pidan {

class DataHeader;

enum class TransactionType { WRITE, READ };

class Transaction {
 public:
  timestamp_t ID() const { return timestamp_; }

  UndoRecord *NewUndoRecordForPut(DataHeader *data_header, const Slice &val);

  TransactionType Type() const { return type_; }

  void AddRead(UndoRecord *undo) { read_set_.insert(undo); }

  void AddWrite(UndoRecord *undo) { write_set_.insert(undo); }

 private:
  // 针对写事务的写集合和读集合，用于在事务提交或终止后释放锁、GC等操作。
  std::set<UndoRecord *> write_set_;
  std::set<UndoRecord *> read_set_;
  TransactionType type_;
  timestamp_t timestamp_;
};
}  // namespace pidan