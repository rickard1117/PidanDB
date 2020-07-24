#pragma once
#include <vector>

#include "common/type.h"
#include "transaction/undo_record.h"

namespace pidan {

enum class TransactionType { WRITE, READ };

class Transaction {
 public:
  txn_id_t ID() const { return id_; }
  UndoRecord *NewUndoRecordForInsert();
  TransactionType Type() const { return type_; }

 private:
  std::vector<UndoRecord *> undo_records_;
  TransactionType type_;
  txn_id_t id_;
};
}  // namespace pidan