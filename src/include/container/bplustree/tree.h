#pragma once

#include <functional>

#include "buffer/buffer_pool_manager.h"
#include "container/bplustree/node.h"

namespace pidan {

// 支持变长key，定长value，非重复key的线程安全B+树。
template <typename KeyType, typename ValueType, typename KeyComparator = std::less<KeyType>>
class BPlusTree {
 public:
  struct ConstIterator {};

 public:
  BPlusTree(BufferPoolManager *buffer_pool_manager) : bpm_(buffer_pool_manager) {}

  // 将迭代器定位到一个指定的位置，迭代器是只读的，不可以用于修改。
  ScanIterator Begin(const KeyType &key);

 private:
  using LNode = LeafNode<KeyType, ValueType>;
  using INode = InnerNode<KeyType, ValueType>;

  // 从node节点开始插入key value
  void StartInsert(Node *node, Node *parent, const KeyType &key, const ValueType &value, Node **split_node,
                   KeyType *split_key) {
    // if (node->IsLeaf()) {
    //   LNode *leaf = static_cast<LNode *>(node);
    //   if (leaf->CheckSpace(key.size())) {
    //     leaf->Insert();
    //   } else {
    //     LNode *sibling = leaf->Split();

    //   }
    // }
  }

 private:
  Node *root_;
  const KeyComparator key_cmp_obj_;
  BufferPoolManager bpm_;
  // const ValueEqualityChecker val_eq_obj_;
};
}  // namespace pidan