#include "db/db_impl.h"

namespace pidan {

Status DBImpl::Put(const Slice &key, const Slice &value) {
  Transaction *txn = txn_manager_.BeginWriteTransaction();
  DataHeader *dh = new DataHeader(txn);
  DataHeader *old_dh = nullptr;

  auto result = index_.InsertUnique(key, dh, &old_dh);
  if (!result) {
    delete dh;
    if (!old_dh->Put(txn, value)) {
      txn_manager_.Abort(txn);
      return Status::FAIL_BY_ACTIVE_TXN;
    }
  } else {
    result = dh->Put(txn, value);
    assert(result);
  }
  txn_manager_.Commit(txn);
  return Status::SUCCESS;
}

Status DBImpl::Get(const Slice &key, std::string *val) {
  Transaction *txn = txn_manager_.BeginReadTransaction();
  DataHeader *dh = nullptr;
  if (!index_.Lookup(key, &dh)) {
    txn_manager_.Abort(txn);
    return Status::KEY_NOT_EXIST;
  }
  bool not_found;
  if (!dh->Select(txn, val, &not_found)) {
    return Status::FAIL_BY_ACTIVE_TXN;
  }
  if (not_found) {
    return Status::KEY_NOT_EXIST;
  }
  return Status::SUCCESS;
}

Status PidanDB::Open(const std::string &name, PidanDB **dbptr) { 
  *dbptr = new DBImpl(); 
  return Status::SUCCESS;
  
  }

}  // namespace pidan
