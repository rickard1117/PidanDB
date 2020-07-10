#pragma once

#include <functional>

#include "common/macros.h"
#include "container/bplustree/node.h"

namespace pidan {

// 支持变长key，定长value，非重复key的线程安全B+树。
template <typename KeyType, typename ValueType, typename KeyComparator = std::less<KeyType>>
class BPlusTree {
 public:
  // struct ConstIterator {};

 public:
  // 将迭代器定位到一个指定的位置，迭代器是只读的，不可以用于修改。
  // ScanIterator Begin(const KeyType &key);

  // 查找key对应的value，找到返回true，否则返回false。
  bool Lookup(const KeyType &key, ValueType *value) const {
    for (;;) {
      Node *node = root_.load();
      bool need_restart = false;
      bool result = StartLookup(node, nullptr, key, value, &need_restart);
      if (need_restart) {
        // 查找失败，需要重启整个流程。
        continue;
      }
      return result;
    }
  }

  // 从节点node开始查找key
  bool StartLookup(const Node *node, const Node *parent, const KeyType &key, ValueType *val, bool *need_restart) const {
    // node->Re
    
  }

 private:
  using LNode = LeafNode<KeyType, ValueType>;
  using INode = InnerNode<KeyType, ValueType>;

  // 从node节点开始插入key value
  // void StartInsert(Node *node, Node *parent, const KeyType &key, const ValueType &value, Node **split_node,
  //                  KeyType *split_key) {
  // if (node->IsLeaf()) {
  //   LNode *leaf = static_cast<LNode *>(node);
  //   if (leaf->CheckSpace(key.size())) {
  //     leaf->Insert();
  //   } else {
  //     LNode *sibling = leaf->Split();

  //   }
  // }
  // }

 private:
  std::atomic<Node *> root_;
  const KeyComparator key_cmp_obj_;
  // const ValueEqualityChecker val_eq_obj_;
};
}  // namespace pidan