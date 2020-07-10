#pragma once

#include "common/config.h"
#include "common/type.h"

namespace pidan {

class Block;
// ValueSlot表示value的存放位置。通过它可以直接访问到一个Value
class ValueSlot {
 public:
  ValueSlot() : pslot_(INVALID_VALUE_SLOT) {}

  ValueSlot(const Block *block, const uint32_t offset) : pslot_(reinterpret_cast<uintptr_t>(block) | offset) {}

  Block *GetBlock() const { return reinterpret_cast<Block *>(pslot_ & (~(static_cast<uintptr_t>(BLOCK_SIZE) - 1))); }

  uint32_t GetOffset() const { return static_cast<uint32_t>(pslot_ & (static_cast<uintptr_t>(BLOCK_SIZE) - 1)); }

  bool IsValid() const { return pslot_ != INVALID_VALUE_SLOT; }

 private:
  uintptr_t pslot_;
};

}  // namespace pidan