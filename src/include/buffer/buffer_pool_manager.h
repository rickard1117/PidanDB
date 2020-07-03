#pragma once

namespace pidan {
class BufferPoolManager {
 public:
  BufferPoolManager(size_t pool_size);

 private:
  size_t pool_size_;
};
}  // namespace pidan
