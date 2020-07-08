#pragma once
#include <vector>

#include "concurrency/transaction.h"
#include "storage/block.h"

namespace pidan {




// DataTable 负责对数据进行增删改查，并且维护多版本以及对事务可见性等信息。
class DataTable {
 public:
  DataTable();

  ValueSlot Insert(Transaction *txn, const std::string &value);

 private:
  std::vector<Block *> blocks_;
};

}  // namespace pidan