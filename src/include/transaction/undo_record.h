#pragma once

namespace pidan {

class DataEntry;
class DataHeader;

enum class UndoRecordType : uint8_t { PUT = 0, DELETE };

class UndoRecord {
 public:
  UndoRecord() = default;

 private:
  UndoRecordType type_;
  std::atomic<timestamp_t> ts_counter_;
  std::atomic<UndoRecord *> next_;
  DataHeader *header_;
  DataEntry data_;
};
}  // namespace pidan