#include "storage/block.h"

#include <cassert>
#include <cstring>

namespace pidan {

void Block::WriteValue(uint32_t pos, const char *data, size_t size) {
  std::memset(&data_[pos], 0, SIZE_VALUE_HEADER);
  std::memcpy(&data_[pos + SIZE_VALUE_HEADER], data, size);
}

void Block::WriteValueIndex(uint32_t pos, uint32_t offset, uint32_t value_size) {
  std::memcpy(&data_[pos], &offset, sizeof(offset));
  std::memcpy(&data_[pos + sizeof(offset)], &value_size, sizeof(value_size));
}

void Block::ReadValueIndex(uint32_t pos, uint32_t *offset, size_t *size) const {
  *offset = *reinterpret_cast<const uint32_t *>(&data_[pos]);
  *size = *reinterpret_cast<const uint32_t *>(&data_[pos + sizeof(*offset)]);
}

void Block::ReadValue(uint32_t pos, size_t size, char *buffer) const {
  std::memcpy(buffer, &data_[pos + SIZE_VALUE_HEADER], size);
}

ValueSlot Block::Insert(Transaction *txn, const char *data, size_t size) {
  (void)txn;
  assert(size > 0);
  // 计算value在DATA区段真正需要的空间。
  size_t value_content_size = SIZE_VALUE_HEADER + size;
  if (static_cast<size_t>(GetFreeSpaceRemaining()) < SIZE_VALUE_INDEX + value_content_size) {
    return ValueSlot();
  }

  uint32_t newFreeSpace = FreeSpaceHeader() - value_content_size;
  SetFreeSpaceHeader(newFreeSpace);
  WriteValue(newFreeSpace, data, size);

  uint32_t value_index_pos = OFFSET_VALUE_OFFSET + ValueCount() * SIZE_VALUE_INDEX;
  assert((value_index_pos & ~(BLOCK_SIZE - 1)) == 0);
  WriteValueIndex(value_index_pos, newFreeSpace, size);
  AddValueCount();
  return ValueSlot(this, value_index_pos);
}

std::string Block::Select(Transaction *txn, const ValueSlot &slot) {
  (void)txn;
  assert(slot.IsValid());
  assert(slot.GetBlock() == this);

  uint32_t val_index_offset = slot.GetOffset();
  assert(val_index_offset < OFFSET_VALUE_OFFSET + ValueCount() * SIZE_VALUE_INDEX);

  uint32_t val_data_offset;
  size_t val_size;
  ReadValueIndex(val_index_offset, &val_data_offset, &val_size);

  std::string val;
  val.resize(val_size);
  ReadValue(val_data_offset, val_size, val.data());
  return val;
}

}  // namespace pidan