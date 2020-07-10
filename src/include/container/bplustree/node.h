#pragma once

#include <atomic>

#include "common/macros.h"

namespace pidan {
/**
 * 因为B+树中key是变长的，value是定长的，所以用key map来存储key和value。
 *
 * Node 格式：
 * ----------------------------------------------
 * | HEADER | ... FREE SPACE ... | ... DATA ... |
 * ----------------------------------------------
 *
 * HEADER格式，以字节为单位：
 * -----------------------------------------------------------------------------------
 * level(2) | slot_use(2) | FreeSpaceOffset(2) | slot1_offset(2) | slot1_size(2)| ...|
 * -----------------------------------------------------------------------------------
 *
 * DATA格式，内部节点和叶子节点的value类型不同，长度也不一样，但都是定长的。
 * --------------------------------
 * FREE SPACE |key2|val2|key1|val1|
 * --------------------------------
 */

// 所有类型的node都不可以构造，必须通过reinterpret_cast而来。
// Node节点中实现了Optimistic Coupling Locking，具体参考论文：The art of practical synchronization
template <typename KeyType>
class Node {
 public:
  bool IsLeaf() const { return level_ == 0; }

  // 为节点加读锁，加锁成功返回true,并且返回当前节点的版本号。
  // 如果加锁失败，返回false。
  bool ReadLockOrRestart(uint64_t *version) const {
    *version = AwaitNodeUnlocked();
    return !IsObsolete(*version);
  }

  // 根据版本号来检查当前节点的锁是否还有效，有效返回true，否则返回false。
  bool CheckOrRestart(uint64_t version) const { return ReadUnlockOrRestart(version); }

  // 释放读锁，如果释放成功，则返回true。否则返回false。
  bool ReadUnlockOrRestart(uint64_t version) const { return version != version_.load(); }

  // 将读锁升级为写锁，升级成功返回true，否则返回false。
  bool UpgradeToWriteLockOrRestart(uint64_t version) {
    return version_.compare_exchange_strong(version, SetLockedBit(version));
  }

  // 加写锁，成功返回true，失败返回false。
  bool WriteLockOrRestart() {
    uint64_t version;
    do {
      bool success = ReadLockOrRestart(&version);
      if (!success) {
        return false;
      }
    } while (!UpgradeToWriteLockOrRestart(version));
  }

  // 释放写锁
  void WriteUnlock() { version_.fetch_add(2); }

  // 释放写锁并将节点标记为删除
  void WriteUnlockObsolete() { version_.fetch_add(3); }

  size_t size() const {}

  KeyType &key(size_t index) {}

 private:
  // spin lock，等待node节点解锁。
  uint64_t AwaitNodeUnlocked() const {
    uint64_t version = version_.load();
    while ((version & 2) == 2) {
      _mm_pause();
      version = version_.load();
    }
    return version;
  }

  // 检查借点是否被标记为删除
  bool IsObsolete(uint64_t version) const { return (version & 1) == 1; }

  // 设置节点写锁标记
  uint64_t SetLockedBit(uint64_t version) { return version + 2; }

  uint16_t level_;
  std::atomic<uint64_t> version_{0b100};
};

template <typename KeyType>
class InnerNode : public Node {
 public:
  Node *GetChild(size_t index) const {}
};

template <typename KeyType, typename ValueType>
class LeafNode : public Node {
 public:
  ValueType &GetValue(size_t index) const {}
};
}  // namespace pidan