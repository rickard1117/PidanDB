#include "transaction/undo_record.h"

#include <cstring>

namespace pidan {

void UndoRecord::GetData(std::string *val) {
  val->resize(data_.size_);
  std::memcpy(&val[0], data_.data_, data_.size_);
}

}  // namespace pidan