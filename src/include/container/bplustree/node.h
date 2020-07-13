#pragma once

#include <atomic>
#include <byte>
#include <functional>

#include "common/macros.h"

namespace pidan {
/**
 * 因为这里实现的B+树中key是变长的，而value是定长的，所以用同一个key map中间层来存储key和value。具体内存布局如下
 *
 * Node 通用格式
 * ----------------------------------------------
 * | HEADER | ... FREE SPACE ... | ... DATA ... |
 * ----------------------------------------------
 *          ^                    ^
 *    free space start      free space end
 *
 *
 * 其中，InnerNode的HEADER部分布局如下，其中最后存储每个key的offset和size的叫做index部分
 * --------------------------------------------------------------------------------------------------------------------
 * level(2) | size(2) | free space start(2) | free space end(2) | version(8) | last_child_pointer(8) | key1_offset(2) |
 * key1_size(2)| ... |
 * --------------------------------------------------------------------------------------------------------------------
 *
 * LeafNode的HEADER布局，和InnerNode一样，最后是index部分：
 * ----------------------------------------------------------------------------------------------------------------
 * level(2) | size(2) | free space start(2) | free space end(2) | version(8) | prev(8) | next(8) | key1_offset(2) |
 * key1_size(2)| ... |
 * ----------------------------------------------------------------------------------------------------------------
 *
 * DATA部分的布局，虽然内部节点和叶子节点的value类型不同，但都是定长的，布局的方法相同，从后向前增长。
 * --------------------------------------------------
 * | ... FREE SPACE ... | key2 | val2 | key1 | val1 |
 * --------------------------------------------------
 */

// 所有类型的node都不可以构造，必须通过reinterpret_cast而来。
// Node节点中实现了Optimistic Coupling Locking，具体参考论文：The art of practical synchronization
// KeyType是一个字节串，支持类似std::string的持size()和data()操作，
// 并具有签名为KeyType(const char *data, size_t size)的构造函数。
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

  // 释放读锁，释放成功返回true。否则返回false。
  bool ReadUnlockOrRestart(uint64_t version) const { return version != version_.load(); }

  // 将读锁升级为写锁，升级成功返回true，否则返回false。
  bool UpgradeToWriteLockOrRestart(uint64_t version) {
    return version_.compare_exchange_strong(version, SetLockedBit(version));
  }

  // 加写锁，加锁成功返回true，否则返回false。
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

  // 返回节点中保存key的数量
  size_t size() const { return size_; }

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

 protected:
  // 返回Node中余下的可用空间，以字节为单位
  size_t FreeSpaceRemaining() { return free_space_end_ - free_space_start_; }

  uint16_t level_;             // 节点所在的level，叶子节点是0, 向上递增。
  uint16_t size_;              // 节点中存储的key的数量，如果是内部节点的话，child数目是size_ + 1
  uint16_t free_space_start_;  // header的结束位置，也是freespace的起始位置，这里只是为了方便计算free
                               // space大小，没有其他作用。
  uint16_t free_space_end_;               // free space的结束位置，从这里进行插入
  std::atomic<uint64_t> version_{0b100};  // 用来作为OLC锁的版本号，具体作用可以参阅论文。
};

using KeyLess = std::function<bool(const Key &key1, const Key &key2)>;

template <typename KeyType>
class InnerNode : public Node {
 public:
  Node *GetChild(size_t index) const {}

  // 向节点中插入一个key,成功返回true,没有足够空间则返回false。
  bool Insert(const KeyType &key, const Node *child, KeyLess key_less) {
    if (!EnoughSpaceForKey(key.size())) {
      return false;
    }
    InsertKeyAndChild();
    return true;
  }

 private:
  // 检查是否有足够的空间来插入大小为key_size的key
  bool EnoughSpaceForKey(size_t key_size) { FreeSpaceRemaining() >= key_size + POINTER_SIZE + SIZE_OFFSET + SIZE_SIZE; }
  
  // 读取指定下标的index
  void ReadIndex(uint16_t index, uint16_t *key_offset, uint16_t *key_size) const {
    uint16_t index_offset = index * (SIZE_OFFSET + SIZE_SIZE);
    *key_offset = *reinterpret_cast<const uint16_t *>(&data_[index_offset]);
    *key_size = *reinterpret_cast<const uint16_t *>(&data_[index_offset + SIZE_OFFSET]);
  }

  // 根据index部分的下标来获取一个key
  const KeyType key(uint16_t index) const {
    assert(index < size_);
    uint16_t key_offset, key_size;
    ReadIndex(&key_offset, &key_size);
    return KeyType(static_cast<const char *>(&data_[key_offset]), key_size);
  }

  // 根据index的下标找到子节点指针
  const Node *GetChildAt(uint16_t idx) const {
    assert(idx <= size_);
    if (idx == size_) {
      return last_child_;
    }
    uint16_t key_offset, key_size;
    ReadIndex(&key_offset, &key_size);

    auto child_offset = *reinterpret_cast<uint16_t *>(&data_[idx * (SIZE_OFFSET + SIZE_SIZE)]);
    return reinterpreter_cast<const Node *>(data_[offset + ])
  }

  // 在Node中找到第一个大于等于target的key，返回它的下标。
  // 如果没找到，返回的是最后一个key的下标 + 1，即包含的key的树目。
  uint16_t FindLower(const KeyType &target, KeyLess key_less) const {
    uint16_t idx = 0;
    while (idx < size_ && key_less(key(idx), key)) {
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

  static constexpr uint32_t DATA_CAPACITY = BPLUSTREE_NODE_SIZE - sizeof(Node) - sizeof(Node *);
  Node *last_child;
  std::byte data_[DATA_CAPACITY];  // 从index部分开始的内存部分都属于data_
};

// template <typename KeyType, typename ValueType>
// class LeafNode : public Node {
//  public:
//   ValueType &GetValue(size_t index) const {}

//  private:
//   static constexpr uint32_t DATA_CAPACITY = BPLUSTREE_NODE_SIZE - sizeof(level_) - sizeof(size_) -
//                                             sizeof(free_space_header_) - sizeof(verion_) - 2 * sizeof(voie *);

//   Node *prev_;
//   Node *next_;
//   std::byte data_[DATA_CAPACITY];
// };

}  // namespace pidan