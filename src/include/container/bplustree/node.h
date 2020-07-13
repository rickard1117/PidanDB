#pragma once

#include <immintrin.h>

#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstring>
#include <functional>
#include <iostream>

#include "common/config.h"
#include "common/macros.h"
#include "common/type.h"

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
 * level(2) | size(2) | free space start(2) | free space end(2) | version(8) | first child(8) | key1_offset(2) |
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

// Node节点中实现了Optimistic Coupling Locking，具体参考论文：The art of practical synchronization
// 模板类型KeyType是一个字节串，支持类似std::string的持size()和data()操作，
// 并具有签名为KeyType(const char *data, size_t size)的构造函数。
// Node是一个抽象类，只有protected的构造函数，不可以直接创建Node类型对象。
// template <typename KeyType>
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
  Node(uint16_t level, uint16_t free_space_start, uint16_t free_space_end)
      : level_(level), free_space_start_(free_space_start), free_space_end_(free_space_end) {}

  // 返回Node中余下的可用空间，以字节为单位
  size_t FreeSpaceRemaining() { return free_space_end_ - free_space_start_; }

  uint16_t level_;             // 节点所在的level，叶子节点是0, 向上递增。
  uint16_t size_{0};           // 节点中存储的key的数量，如果是内部节点的话，child数目是size_ + 1
  uint16_t free_space_start_;  // header的结束位置，也是freespace的起始位置，这里只是为了方便计算free
                               // space大小，没有其他作用。
  uint16_t free_space_end_;               // free space的结束位置，从这里进行插入
  std::atomic<uint64_t> version_{0b100};  // 用来作为OLC锁的版本号，具体作用可以参阅论文。
};

template <typename KeyType>
using BPplusTreeKeyLess = std::function<bool(const KeyType &key1, const KeyType &key2)>;

template <typename KeyType>
class InnerNode : public Node {
 public:
  InnerNode(uint16_t level, Node *left_child, Node *right_child, const KeyType &key)
      : Node(level, 0, DATA_CAPACITY), first_child_(left_child) {
    InsertKeyAndChild(0, key, right_child);
  }

  // 根据key来找到对应的child node指针
  Node *FindChild(const KeyType &key, BPplusTreeKeyLess<KeyType> key_less) {
    uint16_t index = FindLower(key, key_less);
    return GetChildAt(index);
  }

  // 向节点中插入一个key,成功返回true,没有足够空间则返回false。
  bool Insert(const KeyType &key, Node *child, BPplusTreeKeyLess<KeyType> key_less) {
    if (!EnoughSpaceForKey(key.size())) {
      return false;
    }
    uint16_t index = FindLower(key, key_less);
    InsertKeyAndChild(index, key, child);
    return true;
  }

 private:
  void InsertKeyAndChild(uint16_t index, const KeyType &key, Node *child) {
    assert(index <= size_);
    // 不管任何形式的插入都不会改变first_child_的值

    // 首先在DATA区插入key和child
    free_space_end_ -= key.size() + POINTER_SIZE;
    std::memcpy(&data_[free_space_end_], key.data(), key.size());

    std::memcpy(&data_[free_space_end_ + key.size()], &child, POINTER_SIZE);

    // 在index部分插入key offset和key size
    uint16_t index_offset = index * (SIZE_OFFSET + SIZE_SIZE);
    std::copy_backward(&data_[index_offset], &data_[free_space_start_],
                       &data_[free_space_start_ + SIZE_OFFSET + SIZE_SIZE]);
    *reinterpret_cast<uint16_t *>(&data_[index_offset]) = free_space_end_;
    *reinterpret_cast<uint16_t *>(&data_[index_offset + SIZE_OFFSET]) = static_cast<uint16_t>(key.size());

    free_space_start_ += SIZE_OFFSET + SIZE_SIZE;
    size_++;
  }

  // 检查是否有足够的空间来插入大小为key_size的key
  bool EnoughSpaceForKey(size_t key_size) {
    return FreeSpaceRemaining() >= key_size + POINTER_SIZE + SIZE_OFFSET + SIZE_SIZE;
  }

  // 读取指定下标的index
  void ReadIndex(uint16_t index, uint16_t *key_offset, uint16_t *key_size) const {
    uint16_t index_offset = index * (SIZE_OFFSET + SIZE_SIZE);
    *key_offset = *reinterpret_cast<const uint16_t *>(&data_[index_offset]);
    *key_size = *reinterpret_cast<const uint16_t *>(&data_[index_offset + SIZE_OFFSET]);
  }

  // 根据index部分的下标来获取一个key
  KeyType KeyAt(uint16_t index) const {
    assert(index < size_);
    uint16_t key_offset, key_size;
    ReadIndex(index, &key_offset, &key_size);
    return KeyType(reinterpret_cast<const char *>(&data_[key_offset]), key_size);
  }

  // 根据index的下标找到子节点指针
  Node *GetChildAt(uint16_t index) {
    assert(index <= size_);
    if (index == 0) {
      return first_child_;
    }
    uint16_t key_offset, key_size;
    // 因为指针比节点多一个（就是first_child_—），这里要找到上一个key的下标所对应的指针。
    ReadIndex(index - 1, &key_offset, &key_size);
    return *reinterpret_cast<Node **>(&data_[key_offset + key_size]);
  }

  // 在Node中找到第一个大于等于target的key，返回它的下标。
  // 如果没找到，返回的是最后一个key的下标 + 1，即包含的key的树目。
  uint16_t FindLower(const KeyType &target, BPplusTreeKeyLess<KeyType> key_less) const {
    uint16_t idx = 0;
    while (idx < size_ && key_less(KeyAt(idx), target)) {
      idx++;
    }
    return idx;
  }

 private:
  static constexpr uint16_t SIZE_OFFSET = 2;
  static constexpr uint16_t SIZE_SIZE = 2;
  static constexpr uint32_t DATA_CAPACITY = BPLUSTREE_INNERNODE_SIZE - sizeof(Node) - sizeof(Node *);
  // InnerNode中，child指针数目比key多一个，所以用一个额外的指针保存指向最左边孩子节点的指针。
  Node *first_child_;
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