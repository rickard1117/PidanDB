#pragma once
#include <atomic>
#include <cstdint>

#include "common/macros.h"
#include "common/slice.h"
#include "common/type.h"

namespace pidan {
// data entry包含了真正的数据内容，以及指向version chain中下个版本的指针
class DataEntry {
 public:
  DISALLOW_COPY_AND_MOVE(DataEntry);

  // static DataEntry *NewDataEntry(int32_t size, const char *data);

  void Init(const Slice &slice);

 private:
  friend class UndoRecord;
  int64_t size_;

  char data_[0];
};

}  // namespace pidan