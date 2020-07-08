#pragma once

#include "storage/block.h"

namespace pidan {

// BlockStore 用来管理Block，负责对Block进行分配和复用。
class BlockStore {
 public:
  Block *Get() { return new Block; }

 private:
};
}  // namespace pidan