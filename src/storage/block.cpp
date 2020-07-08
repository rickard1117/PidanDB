#include "storage/block.h"

#include <cassert>
#include <cstring>
namespace pidan {

void Block::WriteValue(uint32_t pos, const char *data, size_t size) {
  std::memset(&data_[pos], 0, SIZE_VALUE_HEADER);
  std::memcpy(&data_[pos + SIZE_VALUE_HEADER], data, size);
}

void Block::WriteValueIndex(uint32_t pos, uint32_t offset, uint32_t value_size) {
  std::memcpy(&data_[OFFSET_VALUE_OFFSET + ValueCount() * SIZE_VALUE_INDEX], &offset, sizeof(offset));
  std::memcpy(&data_[OFFSET_VALUE_OFFSET + ValueCount() * 8 + sizeof(offset)], &value_size, sizeof(value_size));
}

ValueSlot Block::Insert(Transaction *txn, const char *data, size_t size) {
  (void)txn;
  // 计算value在DATA区段真正需要的空间。
  size_t value_content_size = SIZE_VALUE_HEADER + size;
  if (static_cast<size_t>(GetFreeSpaceRemaining()) < SIZE_VALUE_INDEX + value_content_size) {
    return ValueSlot();
  }

  uint32_t newFreeSpace = FreeSpaceHeader() - value_content_size;
  SetFreeSpaceHeader(newFreeSpace);
  WriteValue(newFreeSpace, data, size);

  AddValueCount();
  uint32_t value_index_pos = OFFSET_VALUE_OFFSET + ValueCount() * SIZE_VALUE_INDEX;
  assert((value_index_pos & ~(BLOCK_SIZE - 1)) == 0);
  WriteValueIndex(value_index_pos, newFreeSpace, size);

  return ValueSlot(this, value_index_pos);
}



}  // namespace pidan