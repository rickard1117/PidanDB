#pragma once

#include <functional>

#include "common/macros.h"
#include "common/type.h"
#include "container/bplustree/node.h"

namespace pidan {

// 支持变长key，定长value，非重复key的线程安全B+树。
template <typename KeyType, typename ValueType, typename KeyComparator = std::less<KeyType>>
class BPlusTree {
 public:
  BPlusTree() : root_(new LNode), key_cmp_obj_(KeyComparator()) {}

  // 查找key对应的value，找到返回true，否则返回false。
  bool Lookup(const KeyType &key, ValueType *value) const {
    for (;;) {
      Node *node = root_.load();
      bool need_restart = false;
      bool result = StartLookup(node, nullptr, INVALID_OLC_LOCK_VERSION, key, value, &need_restart);
      if (need_restart) {
        // 查找失败，需要重启整个流程。
        continue;
      }
      return result;
    }
  }

  // 插入一对key value，要求key是唯一的。如果key已经存在则返回false，插入成功返回true。
  bool InsertUnique(const KeyType &key, const ValueType &val) {
    Node *node = root_.load();
    if (node->IsLeaf()) {
      LNode *leaf = static_cast<LNode *>(node);
      bool need_split = false;
      if (leaf->InsertUnique(key, key_cmp_obj_, val, &need_split)) {
        return true;
      }
      return false;
    }
    return false;
  }

 private:
  using LNode = LeafNode<KeyType, ValueType>;
  using INode = InnerNode<KeyType>;

  //   bool keyEqual(const KeyType &lhs, const KeyType &rhs) const {
  //     return !key_cmp_obj_(lhs, rhs) && !key_cmp_obj_(rhs, lhs);
  //   }

  //   bool keyLess(const KeyType &lhs, const KeyType &rhs) const { return key_cmp_obj_(lhs, rhs); }

  //   bool keyLessEqual(const KeyType &lhs, const KeyType &rhs) const { return !key_cmp_obj_(rhs, lhs); }

  //   bool keyGreater(const KeyType &lhs, const KeyType &rhs) const { return key_cmp_obj_(rhs, lhs); }

  //   bool keyGreaterEqual(const KeyType &lhs, const KeyType &rhs) const { return !key_cmp_obj_(lhs, rhs); }

  // 从节点node开始查找key，没有找到则返回false
  bool StartLookup(const Node *node, const Node *parent, const uint64_t parent_version, const KeyType &key,
                   ValueType *val, bool *need_restart) const {
    uint64_t version;

    if (!node->ReadLockOrRestart(&version)) {
      std::cerr << "2" << '\n';
      *need_restart = true;
      return false;
    }

    if (parent) {
      if (!parent->ReadUnlockOrRestart(parent_version)) {
        *need_restart = true;
        return false;
      }
    }

    if (node->IsLeaf()) {
      const LNode *leaf = static_cast<const LNode *>(node);
      bool result = leaf->FindValue(key, key_cmp_obj_, val);
      if (!leaf->ReadUnlockOrRestart(version)) {
        *need_restart = true;
        return false;
      }
      *need_restart = false;
      return result;
    }

    const INode *inner = static_cast<const INode *>(node);
    const Node *child = inner->FindChild(key, key_cmp_obj_);
    assert(child != nullptr);
    // 这里需要再次检查，以保证child指针的有效性。
    if (!inner->CheckOrRestart(version)) {
      *need_restart = true;
      return false;
    }
    return StartLookup(child, node, version, key, val, need_restart);
  }

 private:
  std::atomic<Node *> root_;
  const KeyComparator key_cmp_obj_;
  // const ValueEqualityChecker val_eq_obj_;
};
}  // namespace pidan