#pragma once

#include <functional>

#include "buffer/buffer_pool_manager.h"
#include "container/bplustree/node.h"

namespace pidan {

template <typename KeyType, typename ValueType, typename KeyComparator = std::less<KeyType>>
class BPlusTree {
 public:
  struct ConstIterator {};

 public:
  BPlusTree(BufferPoolManager *buffer_pool_manager);

  // 将迭代器定位到一个指定的位置，迭代器是只读的，不可以用于修改。
  ScanIterator Begin(const KeyType &key);
};
}  // namespace pidan