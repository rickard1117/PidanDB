#pragma once

#include <cstddef>
#include <string>

#include "common/config.h"
#include "common/type.h"
#include "concurrency/transaction.h"

namespace pidan {

class Block;
// ValueSlot表示value的存放位置。通过它可以直接访问到一个Value
class ValueSlot {
 public:
  ValueSlot() : pslot_(INVALID_VALUE_SLOT) {}

  ValueSlot(const Block *block, const uint32_t offset) : pslot_(reinterpret_cast<uintptr_t>(block) | offset) {}

  Block *GetBlock() { return reinterpret_cast<Block *>(pslot_ & (~(static_cast<uintptr_t>(BLOCK_SIZE) - 1))); }

  uint32_t GetOffset() { return static_cast<uint32_t>(pslot_ & (static_cast<uintptr_t>(BLOCK_SIZE) - 1)); }

  bool IsValid() const { return pslot_ != INVALID_VALUE_SLOT; }

 private:
  uintptr_t pslot_;
};

/**
 * Block 内存布局
 * ---------------------------------------
 * | HEADER | ...FREE SPACE... | ...DATA |
 * ---------------------------------------
 *                             ^
 *                        free space header
 *
 *
 * 其中HEADER的布局：
 * ----------------------------------------------------------------------------------------
 * | value count(32) | free space header(32) | value1 offset(32) | value1 size(32) | ... |
 * ----------------------------------------------------------------------------------------
 *
 * DATA的布局：
 * |...| value2 version chain (64) | value2 | value1 version chain (64) | value1 |
 */

// Block是一块内存，用于存放数据
class alignas(BLOCK_SIZE) Block {
 public:
  Block() {
    SetFreeSpaceHeader(BLOCK_SIZE);
    SetValueCount(0);
  }
  // 返回当前block中存储的value数量
  uint32_t ValueCount() const { return *reinterpret_cast<const uint32_t *>(&data_[OFFSET_VALUE_COUNT]); }

  // 插入value，成功返回true，否则返回flase。
  ValueSlot Insert(Transaction *txn, const char *data, size_t size);

  // 根据ValueSlot在Block中找到一个value
  std::string Select(Transaction *txn, const ValueSlot &slot);

 private:
  // 每个value头部的大小(byte)
  static constexpr size_t SIZE_VALUE_HEADER = 8;
  // value count字段的大小
  static constexpr size_t SIZE_VALUE_COUNT = 4;
  // free space offset字段大小
  static constexpr size_t SIZE_SPACE_OFFSET = 4;
  // value 索引的大小，即 value offset + value size这两个字段所占的内存大小
  static constexpr size_t SIZE_VALUE_INDEX = 8;
  // value count的偏移量
  static constexpr size_t OFFSET_VALUE_COUNT = 0;
  // free space的偏移量
  static constexpr size_t OFFSET_FREE_SPACE = 4;
  // value offset的偏移量，从这个位置开始，后面会存放所有value的offset和size
  static constexpr size_t OFFSET_VALUE_OFFSET = 8;

  uint32_t FreeSpaceHeader() const { return *reinterpret_cast<const uint32_t *>(&data_[OFFSET_FREE_SPACE]); }

  uint32_t HeaderSize() const { return SIZE_VALUE_COUNT + SIZE_SPACE_OFFSET + ValueCount() * SIZE_VALUE_INDEX; }

  uint32_t GetFreeSpaceRemaining() const { return FreeSpaceHeader() - HeaderSize(); }

  void SetFreeSpaceHeader(uint32_t free_space_header) {
    *reinterpret_cast<uint32_t *>(&data_[OFFSET_FREE_SPACE]) = free_space_header;
  }

  void SetValueCount(uint32_t count) { *reinterpret_cast<uint32_t *>(&data_[OFFSET_VALUE_COUNT]) = count; }

  void AddValueCount() { *reinterpret_cast<uint32_t *>(&data_[OFFSET_VALUE_COUNT]) += 1; }

  // 向指定位置写入value,包括value header。
  void WriteValue(uint32_t pos, const char *value, size_t size);

  // 写入value index，包括value数据的偏移量和大小。
  void WriteValueIndex(uint32_t pos, uint32_t offset, uint32_t value_size);

  std::byte data_[BLOCK_SIZE];
};

}  // namespace pidan