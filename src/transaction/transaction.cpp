#include "transaction/transaction.h"

#include <cassert>

#include "storage/data_header.h"
#include "transaction/undo_record.h"

namespace pidan {

UndoRecord *Transaction::NewUndoRecordForPut(DataHeader *data_header, const Slice &val) {
  auto *buf = new char[sizeof(UndoRecord) + val.size()];
  auto *record = reinterpret_cast<UndoRecord *>(buf);
  record->type_ = UndoRecordType::PUT;
  record->timestamp_ = MAX_TIMESTAMP;
  record->next_ = nullptr;
  record->header_ = data_header;
  record->data_.Init(val);
  write_set_.push_back(record);
  return record;
}

void Transaction::ReadLockOn(DataHeader *data_header) {
  auto result = read_lock_set_.insert(data_header);
  assert(result.second);
}

void Transaction::WriteLockOn(DataHeader *data_header) {
  auto result = write_lock_set_.insert(data_header);
  assert(result.second);
}

bool Transaction::AlreadyWriteLockOn(DataHeader *data_header) {
  return write_lock_set_.find(data_header) != write_lock_set_.cend();
}

bool Transaction::AlreadyReadLockOn(DataHeader *data_header) {
  return read_lock_set_.find(data_header) != read_lock_set_.cend();
}

void Transaction::UpgradeToWriteLock(DataHeader *data_header) {
  auto size = read_lock_set_.erase(data_header);
  assert(size == 1);
  auto result = write_lock_set_.insert(data_header);
  assert(result.second);
}

void Transaction::MakeWriteVisible(timestamp_t timestamp) {
  for (auto *record : write_set_) {
    record->SetTimestamp(timestamp);
  }
  RelaseAllWriteLock();
}

// 释放所有读锁，用于事务提交。
void Transaction::RealseAllReadLock() {
  for (auto *dh : read_lock_set_) {
    dh->latch_.ReadUnlock();
  }
}

void Transaction::RelaseAllWriteLock() {
  for (auto *dh : write_lock_set_) {
    dh->latch_.WriteUnlock();
  }
}

}  // namespace pidan