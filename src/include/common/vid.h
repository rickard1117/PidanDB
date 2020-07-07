#pragma once

#include "common/type.h"

namespace pidan {

// value id，用来做为value的索引。
class VID {
 public:
  VID() = default;

 private:
  page_id_t page_id_ = INVALID_PAGE_ID;
  uint32_t slot_num_ = 0;
};
}  // namespace pidan