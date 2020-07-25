#include "transaction/transaction.h"

#include "transaction/undo_record.h"

namespace pidan {

UndoRecord *Transaction::NewUndoRecordForPut(DataHeader *data_header, const Slice &val) {
  auto buf = new char[sizeof(UndoRecord) + val.size()];
  auto record = reinterpret_cast<UndoRecord *>(buf);
  record->type_ = UndoRecordType::PUT;
  record->timestamp_ = MAX_TIMESTAMP;
  record->next_ = nullptr;
  record->header_ = data_header;
  record->data_.Init(val);
  return record;
}

}  // namespace pidan