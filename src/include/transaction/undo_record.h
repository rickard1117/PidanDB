#pragma once

#include <atomic>
#include <string>
#include "common/type.h"
#include "storage/data_entry.h"

namespace pidan {

class DataEntry;
class DataHeader;

enum class UndoRecordType : uint8_t { PUT = 0, DELETE };

class UndoRecord {
 public:
  std::atomic<UndoRecord *> &Next() { return next_; }

  bool NewerThan(timestamp_t timestamp) {
    return timestamp_ > timestamp;
  }

  UndoRecordType Type() const {
    return type_;
  }

  void GetData(std::string *val);

 private:
  friend class Transaction;
  UndoRecord() = default;

  std::atomic<timestamp_t> timestamp_;
  std::atomic<UndoRecord *> next_;
  DataHeader *header_;
  UndoRecordType type_;
  DataEntry data_;
};

}  // namespace pidan