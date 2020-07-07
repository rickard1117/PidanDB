#pragma once

#include <stdint.h>

#include "common/macros.h"

#ifndef BYTE_SIZE
#define BYTE_SIZE 8U
#endif

namespace pidan {
class RawBitMap {
 public:
  MEM_REINTERPRET_CAST_ONLY(RawBitMap);

  // 将bit位数转换为相应的字节数，向上取整。
  static constexpr uint32_t SizeInBytes(uint32_t n) { return n % BYTE_SIZE == 0 ? n / BYTE_SIZE : n / BYTE_SIZE + 1; }

  // 在一块大小为nbytes的连续内存mem块的头部创建一个RawBitMap。
  // 返回bitmap占用的字节数，如果没有足够的空间，操作会失败，返回nullptr。
  static RawBitMap *CreateBitMap(void *mem, uint32_t nbytes, uint32_t slots);

  // 检查第pos位置上的bit。1返回true，0返回false。
  bool Test(const uint32_t pos) {
    //   bits_[pos / BYTE_SIZE] & (1 << (pos % BYTE_SIZE));
    return false;
  }

  private:
    uint32_t size_;
    char bits_[0];
};
}  // namespace pidan