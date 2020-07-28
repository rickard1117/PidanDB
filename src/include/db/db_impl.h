#pragma once

#include "container/bplustree/tree.h"
#include "pidan/db.h"
#include "storage/data_header.h"
#include "transaction/transaction_manager.h"

namespace pidan {

class DBImpl : public PidanDB {
 public:
  DBImpl() : txn_manager_(&ts_manager_) {}

  virtual Status Put(const Slice &key, const Slice &value) override;

  virtual Status Get(const Slice &key, std::string *val) override;

  virtual Status Delete(const Slice &key) override {}

  virtual Status Begin() override {}

  virtual Status Commit() override {}

  virtual Status Abort() override {}

 private:
  BPlusTree<Slice, DataHeader *> index_;
  TimestampManager ts_manager_;
  TransactionManager txn_manager_;
};

}  // namespace pidan