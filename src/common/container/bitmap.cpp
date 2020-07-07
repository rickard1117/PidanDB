#include "common/container/bitmap.h"

#include <cstring>
namespace pidan {

RawBitMap *RawBitMap::CreateBitMap(void *mem, uint32_t nbytes, uint32_t slots) {
  auto size = SizeInBytes(slots) + 4;  // size_字段占4个字节
  if (size > nbytes) {
    return nullptr;
  }
  std::memset(mem, 0, size);
  RawBitMap *bitmap = reinterpret_cast<RawBitMap *>(mem);
  bitmap->size_ = slots;
  return bitmap;
}




}  // namespace pidan