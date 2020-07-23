#pragma once

#include "common/macros.h"
#include "common/slice.h"
#include "common/type.h"
#include "storage/data_entry.h"
#include "transaction/transaction.h"

namespace pidan {

// DataHeader 中保存了一条数据所有的多版本信息
class DataHeader {
 public:
  DISALLOW_COPY_AND_MOVE(DataHeader);

  DataHeader() : version_chain_(nullptr) {}

  // 插入一个新的值
  bool Insert(Transaction *txn, const Slice val);

 private:
  
  // txn_id_表示了当前正在修改此数据项的事务，可以当做此数据项的latch_
  //   std::atomic<uint64_t> latch_flag_;
  std::atomic<DataEntry *> version_chain_;
};

}  // namespace pidan