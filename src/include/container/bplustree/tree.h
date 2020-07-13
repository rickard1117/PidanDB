#pragma once

#include <functional>

#include "common/macros.h"
#include "common/type.h"
#include "container/bplustree/node.h"

namespace pidan {

// 支持变长key，定长value，非重复key的线程安全B+树。
// template <typename KeyType, typename ValueType, typename KeyComparator = std::less<KeyType>>
// class BPlusTree {
//  public:
//   // struct ConstIterator {};

//  public:
//   // 将迭代器定位到一个指定的位置，迭代器是只读的，不可以用于修改。
//   // ScanIterator Begin(const KeyType &key);

//   // 查找key对应的value，找到返回true，否则返回false。
//   bool Lookup(const KeyType &key, ValueType *value) const {
//     for (;;) {
//       Node *node = root_.load();
//       bool need_restart = false;
//       bool result = StartLookup(node, nullptr, INVALID_OLC_LOCK_VERSION, key, value, &need_restart);
//       if (need_restart) {
//         // 查找失败，需要重启整个流程。
//         continue;
//       }
//       return result;
//     }
//   }

//  private:
//   using LNode = LeafNode<KeyType, ValueType>;
//   using INode = InnerNode<KeyType, ValueType>;

//   bool keyEqual(const KeyType &lhs, const KeyType &rhs) const {
//     return !key_cmp_obj_(lhs, rhs) && !key_cmp_obj_(rhs, lhs);
//   }

//   bool keyLess(const KeyType &lhs, const KeyType &rhs) const { return key_cmp_obj_(lhs, rhs); }

//   bool keyLessEqual(const KeyType &lhs, const KeyType &rhs) const { return !key_cmp_obj_(rhs, lhs); }

//   bool keyGreater(const KeyType &lhs, const KeyType &rhs) const { return key_cmp_obj_(rhs, lhs); }

//   bool keyGreaterEqual(const KeyType &lhs, const KeyType &rhs) const { return !key_cmp_obj_(lhs, rhs); }

//   template <typename NodeType>
//   size_t FindLower(const NodeType *node, const KeyType &key) {
//     size_t idx = 0;
//     while (node->size() > idx && keyEqual(node->key(idx), key)) {
//       idx++;
//     }
//     return idx;
//   }

//   // 从节点node开始查找key
//   bool StartLookup(const Node *node, const Node *parent, const uint64_t parent_version, const KeyType &key,
//                    ValueType *val, bool *need_restart) const {
//     uint64_t version;
//     if (!node->ReadLockOrRestart(&version)) {
//       *need_restart = true;
//       return false;
//     }

//     if (parent) {
//       if (!parent->ReadUnlockOrRestart(parent_version)) {
//         *need_restart = true;
//         return false;
//       }
//     }

//     if (node->IsLeaf()) {
//       const LNode *leaf = static_cast<const LNode *>(node);
//       size_t index = leaf->FindLower(key);

//       if (index > leaf->size()) {
//         if (!leaf->ReadUnlockOrRestart(version)) {
//           *need_restart = true;
//         } else {
//           *need_restart = false;
//         }
//         return false;
//       }

//       *val = leaf->GetValue(index);
//       if (!leaf->ReadUnlockOrRestart(version)) {
//         *need_restart = true;
//         return false;
//       }
//       *need_restart = false;
//       return true;
//     }

//     const INode *inner = static_cast<const INode *>(node);
//     const Node *child = inner->GetChild(inner->FindLower(key));
//     // 这里需要再次检查，以保证child指针的有效性。
//     if (!inner->CheckOrRestart(version)) {
//       *need_restart = true;
//       return false;
//     }
//     return StartLookup(child, node, version, key, val, need_start);
//   }

//  private:
//   std::atomic<Node *> root_;
//   const KeyComparator key_cmp_obj_;
//   // const ValueEqualityChecker val_eq_obj_;
// };
}  // namespace pidan