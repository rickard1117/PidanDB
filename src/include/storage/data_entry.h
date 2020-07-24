#pragma once
#include <cstdint>

#include "common/macros.h"
#include "common/type.h"
#include <atomic>

namespace pidan {
// data entry包含了真正的数据内容，以及指向version chain中下个版本的指针
class DataEntry {
 public:
  MEM_REINTERPRET_CAST_ONLY(DataEntry);

  static DataEntry *NewDataEntry(int32_t size, const char *data);

  
 private:
  int64_t size_;
  
  char data_[0];
};

}  // namespace pidan