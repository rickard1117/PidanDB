#include "storage/data_entry.h"

namespace pidan {

void DataEntry::Init(const Slice &slice) {
  size_ = slice.size();
  std::memcpy(data_, slice.data(), size_);
}

}  // namespace pidan