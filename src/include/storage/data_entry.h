#pragma once
#include <atomic>
#include <cstdint>
#include <string>

#include "common/macros.h"
#include "common/type.h"
#include "pidan/slice.h"

namespace pidan {
// data entry包含了真正的数据内容，以及指向version chain中下个版本的指针
class DataEntry {
 public:
  DISALLOW_COPY_AND_MOVE(DataEntry);

  void Init(const Slice &slice);

  // only for test
  std::string ToString() const { return std::string(data_, size_); }

 private:
  friend class UndoRecord;
  int64_t size_;

  char data_[0];
};

}  // namespace pidan