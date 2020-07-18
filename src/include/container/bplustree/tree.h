#pragma once

#include <functional>
#include <iostream>

#include "common/macros.h"
#include "common/type.h"
#include "container/bplustree/node.h"

namespace pidan {

// 支持变长key，定长value，非重复key的线程安全B+树。
template <typename KeyType, typename ValueType>
class BPlusTree {
 public:
  BPlusTree() : root_(new LNode) {}

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
  bool InsertUnique(const KeyType &key, const ValueType &value) {
    for (;;) {
      Node *node = root_.load();
      bool need_restart = false;
      bool result = StartInsertUnique(node, nullptr, INVALID_OLC_LOCK_VERSION, key, value, &need_restart);
      if (need_restart) {
        std::cerr<< "need restart" << '\n';
        continue;
      }
      return result;
    }
  }

 private:
  using LNode = LeafNode<KeyType, ValueType>;
  using INode = InnerNode<KeyType>;

  // 从node节点开始，向树中插入key value，插入失败返回false，否则返回true。
  bool StartInsertUnique(Node *node, INode *parent, const uint64_t parent_version, const KeyType &key,
                         const ValueType &val, bool *need_restart) {
    uint64_t version;
    if (!node->ReadLockOrRestart(&version)) {
      std::cerr << __LINE__ << '\n';
      *need_restart = true;
      return false;
    }

    if (!node->IsLeaf()) {
      INode *inner = static_cast<INode *>(node);
      if (!inner->EnoughSpaceFor(MAX_KEY_SIZE)) {
        // 节点空间不足，要分裂。
        if (parent) {
          if (!parent->UpgradeToWriteLockOrRestart(parent_version)) {
            std::cerr << __LINE__ << '\n';
            *need_restart = true;
            return false;
          }
        }

        if (!inner->UpgradeToWriteLockOrRestart(version)) {
          if (parent) {
            parent->WriteUnlock();
          }
          std::cerr << __LINE__ << '\n';
          *need_restart = true;
          return false;
        }

        // TODO: 这个地方有疑问，到底应不应该判断。
        if (parent == nullptr && (inner != root_)) {
          // node原本是根节点，但是同时有其他线程在此线程对根节点加写锁之前已经将根节点分裂或删除了
          // 此时虽然加写锁可以成功，但根节点已经是新的节点了，因此要重启。
          inner->WriteUnlock();
          std::cerr << __LINE__ << '\n';
          *need_restart = true;
          return false;
        }

        KeyType split_key;
        INode *sibling = inner->Split(&split_key);
        if (parent) {
          bool result = parent->Insert(split_key, sibling);
          assert(result);
        } else {
          root_ = new INode(inner->level() + 1, inner, sibling, split_key);
        }
        inner->WriteUnlock();
        if (parent) {
          parent->WriteUnlock();
        }
        // 分裂完毕，重新开始插入流程。
        std::cerr << __LINE__ << '\n';
        *need_restart = true;
        return false;
      }

      if (parent) {
        if (!parent->ReadUnlockOrRestart(parent_version)) {
          std::cerr << __LINE__ << '\n';
          *need_restart = true;
          return false;
        }
      }

      Node *child = inner->FindChild(key);
      if (!inner->CheckOrRestart(version)) {
        std::cerr << __LINE__ << '\n';
        *need_restart = true;
        return false;
      }
      return StartInsertUnique(child, inner, version, key, val, need_restart);
    }

    LNode *leaf = static_cast<LNode *>(node);
    if (leaf->Exists(key)) {
      if (!leaf->ReadUnlockOrRestart(version)) {
        std::cerr << __LINE__ << '\n';
        *need_restart = true;
      } else {
        *need_restart = false;
      }
      return false;
    }

    if (!leaf->EnoughSpaceFor(MAX_KEY_SIZE)) {
      if (parent) {
        // leaf节点要分裂，回向父节点插入key，要先拿到父节点的写锁。
        // 之前访问父节点已经保证了父节点的空间足够，如果在访问后父节点发生了改动，那么这里会加锁失败。
        if (!parent->UpgradeToWriteLockOrRestart(parent_version)) {
          std::cerr << __LINE__ << '\n';
          *need_restart = true;
          return false;
        }
      }

      if (!leaf->UpgradeToWriteLockOrRestart(version)) {
        std::cerr << __LINE__ << '\n';
        *need_restart = true;
        if (parent) {
          parent->WriteUnlock();
        }
        return false;
      }

      // TODO: 这个地方有疑问，到底应不应该判断。
      if (parent == nullptr && (leaf != root_)) {
        // node原本是根节点，但是同时有其他线程在此线程对根节点加写锁之前已经将根节点分裂或删除了
        // 此时虽然加写锁可以成功，但根节点已经是新的节点了，因此要重启。
        leaf->WriteUnlock();
        std::cerr << __LINE__ << '\n';
        *need_restart = true;
        return false;
      }

      KeyType split_key;
      LNode *sibling = leaf->Split(&split_key);
      if (parent) {
        bool result = parent->Insert(split_key, sibling);
        assert(result);
      } else {
        // 当前节点是leaf node，那么父节点的level必须是1
        root_ = new INode(1, leaf, sibling, split_key);
      }
      // TODO : 分裂完毕了，此时是否可以直接将key插入到leaf或者sibling节点了，还是需要再重启一次？
      if (key < split_key) {
        leaf->Insert(key, val);
      } else {
        sibling->Insert(key, val);
      }
      leaf->WriteUnlock();
      if (parent) {
        parent->WriteUnlock();
      }
      *need_restart = false;
      return true;
    } else {
      // leaf 节点空间足够，直接插入不需要再对父节点加写锁了
      if (!leaf->UpgradeToWriteLockOrRestart(version)) {
        std::cerr << __LINE__ << '\n';
        *need_restart = true;
        return false;
      }
      if (parent) {
        if (!parent->ReadUnlockOrRestart(parent_version)) {
          std::cerr << __LINE__ << '\n';
          *need_restart = true;
          return false;
        }
      }
      leaf->Insert(key, val);
      leaf->WriteUnlock();
      return true;
    }
  }

  // 从节点node开始查找key，没有找到则返回false
  bool StartLookup(const Node *node, const Node *parent, const uint64_t parent_version, const KeyType &key,
                   ValueType *val, bool *need_restart) const {
    uint64_t version;

    if (!node->ReadLockOrRestart(&version)) {
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
      bool result = leaf->FindValue(key, val);
      if (!leaf->ReadUnlockOrRestart(version)) {
        *need_restart = true;
        return false;
      }
      *need_restart = false;
      return result;
    }

    const INode *inner = static_cast<const INode *>(node);
    const Node *child = inner->FindChild(key);
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
  // const KeyComparator key_cmp_obj_;
};
}  // namespace pidan