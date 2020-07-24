#include "transaction/transaction.h"

#include "transaction/undo_record.h"

namespace pidan {

UndoRecord *Transaction::NewUndoRecord() {
     auto record = new UndoRecord();
     undo_records_.push_back(record);
     return record;
}

}  // namespace pidan