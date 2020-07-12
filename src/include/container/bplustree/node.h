#pragma once

#include <atomic>
#include <byte>
#include <functional>

#include "common/macros.h"

namespace pidan {
/**
 * 因为B+树中key是变长的，value是定长的，所以用key map来存储key和value。
 *
 * Node 通用格式：
 * ----------------------------------------------
 * | HEADER | ... FREE SPACE ... | ... DATA ... |
 * ----------------------------------------------
 *                               ^
 *                        free space offset
 * 
 * 其中，InnerNode的HEADER部分格式，以字节为单位：
 * ------------------------------------------------------------------------------------------------------------------------------------
 * level(2) | size(2) | free space offset(2) | padding(2) | version(8) | last_child_pointer(8) | key1_offset(2) | key1_size(2)| ... |
 * ------------------------------------------------------------------------------------------------------------------------------------
 *
 * LeafNode的HEADER格式：
 * --------------------------------------------------------------------------------------------------------------------------------
 * level(2) | size(2) | free space offset(2) | padding(2) | version(8) | prev(8) | next(8) | key1_offset(2) | key1_size(2)| ... |
 * --------------------------------------------------------------------------------------------------------------------------------
 * 
 * DATA格式，虽然内部节点和叶子节点的value类型不同，但都是定长的。
 * ------------------------------------------
 * | FREE SPACE | key2 | val2 | key1 | val1 |
 * ------------------------------------------
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

  size_t size() const { return size_; }

  // KeyType &key(size_t index) {}

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

  //
  bool InsertHeaderKey(uint32_t size);

  // void Insert();
  size_t FreeSpaceRemaining() { return free_space_header_ - index_header_; }

 protected:
  uint16_t level_;
  uint16_t size_;  // 存储key的数量
  uint16_t index_header_;
  uint16_t free_space_offset_;
  std::atomic<uint64_t> version_{0b100};
  // std::bate data_[DATA_CAPACITY];
};

using KeyLess = std::function<bool(const Key &key1, const Key &key2)>;

template <typename KeyType>
class InnerNode : public Node {
 public:
  Node *GetChild(size_t index) const {}

  // 向节点中插入一个key,成功返回true,没有足够空间则false。
  bool Insert(const KeyType &key, const Node *child, KeyLess key_less) {
    if (!EnoughSpaceForKey(key.size())) {
      return false;
    }
    InsertKeyAndChild();
    return true;
  }

  bool EnoughSpaceForKey(size_t key_size) { FreeSpaceRemaining() >= key_size + POINTER_SIZE + SIZE_OFFSET + SIZE_SIZE; }

 private:
  const KeyType key(uint16_t index) {
    assert(index < size_);
    data_[]
  }

  const KeyType key(uint16_t offset, uint16_t size) const {
    assert(offset < index_header_);
    return KeyType(&data_[offset], size);
  }
  
  // 根据index的下标找到子节点指针
  const Node *GetChildAt(uint16_t idx) const {
    assert(idx <= size_);
    if(idx == size_) {
      return last_child;
    }

    auto offset = *reinterpret_cast<uint16_t *>(&data_[idx * (SIZE_OFFSET + SIZE_SIZE)]);
    return reinterpreter_cast<const Node*>(data_[offset + ])
  }

  uint16_t FindLower(const KeyType &key, KeyLess key_less) const {
    uint16_t idx = 0;
    while(idx < size_ && key_less(key(idx), key)) {
      idx++;
    }
    return idx;
  }

  void InsertKeyValue(const KeyType &key, const Node *child) {
    uint16_t key_size = static_cast<uint16_t>(key.size());
    free_space_offset_ -= POINTER_SIZE + key_size;
    std::memcpy(&data_[free_space_offset_], key.data(), key_size);
    std::memcpy(&data_[free_space_offset_ + key_size], child, POINTER_SIZE);
    *reinterpret_cast<uint16_t *>(&data_[index_header_]) = free_space_offset_;
    *reinterpret_cast<uint16_t *>(&data_[index_header_ + SIZE_OFFSET]) = key_size;
    index_header_ += 4;
  }

  // void InsertChild(Node *node) {
  //   free_space_offset_ -= POINTER_SIZE;
  //   data_[free_space_offset_]
  // }
 private:
  static constexpr uint16_t SIZE_OFFSET = 2;
  static constexpr uint16_t SIZE_SIZE = 2;
  // static constexpr uint16_t SIZE_VALUE = POINTER_SIZE;

  static constexpr uint32_t DATA_CAPACITY =
      BPLUSTREE_NODE_SIZE - sizeof(Node) - sizeof(Node *);
  Node *last_child;
  std::byte data_[DATA_CAPACITY];
};

template <typename KeyType, typename ValueType>
class LeafNode : public Node {
 public:
  ValueType &GetValue(size_t index) const {}

 private:
  static constexpr uint32_t DATA_CAPACITY = BPLUSTREE_NODE_SIZE - sizeof(level_) - sizeof(size_) -
                                            sizeof(free_space_header_) - sizeof(verion_) - 2 * sizeof(voie *);

  Node *prev_;
  Node *next_;
  std::byte data_[DATA_CAPACITY];
};

}  // namespace pidan