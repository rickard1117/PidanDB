#pragma once

#include <immintrin.h>

#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstring>
#include <functional>

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
  Node(uint16_t level) : level_(level), size_(0), version_(0b100) {}
  uint16_t level_;  // 节点所在的level，叶子节点是0, 向上递增。
  uint16_t size_;   // 节点中存储的key的数量，如果是内部节点的话，child数目是size_ + 1
  std::atomic<uint64_t> version_;  // 用来作为OLC锁的版本号，具体作用可以参阅论文。
};

template <typename KeyType>
using KeyLess = std::function<bool(const KeyType &key1, const KeyType &key2)>;

template <typename KeyType>
bool key_equal(KeyLess<KeyType> key_less, const KeyType &key1, const KeyType &key2) {
  return !key_less(key1, key2) && !key_less(key2, key1);
}

template <typename KeyType, typename ValueType, uint32_t SIZE>
class KeyMap {
 public:
  KeyMap() : free_space_start_(0), free_space_end_(SIZE), size_(0) {}

  // 在keymap中找到第一个不小于key的下标。下标的最小值为0，最大值为Size()的返回值。
  uint16_t FindLower(const KeyType &key, KeyLess<KeyType> key_less) const {
    uint16_t idx = 0;
    while (idx < size_ && key_less(KeyAt(idx), key)) {
      idx++;
    }
    return idx;
  }

  KeyType KeyAt(uint16_t index) const {
    assert(index < size_);
    uint16_t key_offset, key_size;
    ReadIndex(index, &key_offset, &key_size);
    return KeyType(reinterpret_cast<const char *>(&data_[key_offset]), key_size);
  }

  ValueType ValueAt(uint16_t index) {
    assert(index < size_);
    uint16_t key_offset, key_size;
    ReadIndex(index, &key_offset, &key_size);
    return *reinterpret_cast<ValueType *>(&data_[key_offset + key_size]);
  }

  KeyType KeyValueAt(uint16_t index, ValueType *val) {
    assert(index < size_);
    uint16_t key_offset, key_size;
    ReadIndex(index, &key_offset, &key_size);
    *val = *reinterpret_cast<ValueType *>(&data_[key_offset + key_size]);
    return KeyType(reinterpret_cast<const char *>(&data_[key_offset]), key_size);
  }

  // 在下标为index的位置插入key和value
  void InsertKeyValue(uint16_t index, const KeyType &key, const ValueType &val) {
    assert(index <= size_);

    // 插入key和value
    free_space_end_ -= key.size() + SIZE_VALUE;
    std::memcpy(&data_[free_space_end_], key.data(), key.size());

    std::memcpy(&data_[free_space_end_ + key.size()], &val, SIZE_VALUE);

    // 在index部分插入key offset和key size
    uint16_t index_offset = index * (SIZE_OFFSET + SIZE_SIZE);
    std::copy_backward(&data_[index_offset], &data_[free_space_start_],
                       &data_[free_space_start_ + SIZE_OFFSET + SIZE_SIZE]);
    *reinterpret_cast<uint16_t *>(&data_[index_offset]) = free_space_end_;
    *reinterpret_cast<uint16_t *>(&data_[index_offset + SIZE_OFFSET]) = static_cast<uint16_t>(key.size());

    free_space_start_ += SIZE_OFFSET + SIZE_SIZE;
    assert(free_space_start_ <= free_space_end_);
    size_++;
  }

  // 检查是否还有足够的空间可以插入大小为key_size的key以及一个定长的value
  bool EnoughSpace(size_t key_size) { return FreeSpaceRemaining() >= key_size + SIZE_VALUE + SIZE_OFFSET + SIZE_SIZE; }

  uint16_t Size() const { return size_; }

 private:
  void ReadIndex(uint16_t index, uint16_t *key_offset, uint16_t *key_size) const {
    uint16_t index_offset = index * (SIZE_OFFSET + SIZE_SIZE);
    *key_offset = *reinterpret_cast<const uint16_t *>(&data_[index_offset]);
    *key_size = *reinterpret_cast<const uint16_t *>(&data_[index_offset + SIZE_OFFSET]);
  }

  uint16_t FreeSpaceRemaining() { return free_space_end_ - free_space_start_; }

  static constexpr uint16_t SIZE_OFFSET = 2;
  static constexpr uint16_t SIZE_SIZE = 2;
  static constexpr uint16_t SIZE_VALUE = sizeof(ValueType);

  uint16_t free_space_start_;  // header的结束位置，也是freespace的起始位置，这里只是为了方便计算free
                               // space大小，没有其他作用。
  uint16_t free_space_end_;  // free space的结束位置，从这里进行插入
  uint16_t size_;
  std::byte data_[SIZE];
};

template <typename KeyType>
class InnerNode : public Node {
 public:
  InnerNode(uint16_t level, Node *left_child, Node *right_child, const KeyType &key)
      : Node(level), first_child_(left_child) {
    key_map_.InsertKeyValue(0, key, right_child);
  }

  // 根据key来找到对应的child node指针
  Node *FindChild(const KeyType &key, KeyLess<KeyType> key_less) {
    uint16_t index = key_map_.FindLower(key, key_less);
    if (index == 0) {
      return first_child_;
    }
    // 因为first_child_没有保存在KeyMap中，所以这里要用index - 1去寻找child指针
    return key_map_.ValueAt(index - 1);
  }

  // 向节点中插入一个key,成功返回true,没有足够空间则返回false。
  bool Insert(const KeyType &key, Node *child, KeyLess<KeyType> key_less) {
    if (!key_map_.EnoughSpace(key.size())) {
      return false;
    }
    uint16_t index = key_map_.FindLower(key, key_less);
    key_map_.InsertKeyValue(index, key, child);
    return true;
  }

  uint16_t Size() const { return size_; }

 private:
  // InnerNode中，child指针数目比key多一个，所以用一个额外的指针保存指向最左边孩子节点的指针。
  Node *first_child_;
  KeyMap<KeyType, Node *, BPLUSTREE_INNERNODE_SIZE - sizeof(Node) - POINTER_SIZE> key_map_;
};

template <typename KeyType, typename ValueType>
class LeafNode : public Node {
 public:
  LeafNode() : Node(0), prev_(nullptr), next_(nullptr) {}

  // 查找key所对应的val，如果不存在就返回false
  bool FindValue(const KeyType &key, KeyLess<KeyType> key_less, ValueType *val) {
    uint16_t index = key_map_.FindLower(key, key_less);
    if (index >= key_map_.Size()) {
      return false;
    }
    KeyType result_key = key_map_.KeyValueAt(index, val);
    return key_equal(key_less, result_key, key);
  }

  // 插入key value，如果key已经存在，则插入失败。
  bool InsertUnique(const KeyType &key, KeyLess<KeyType> key_less, const ValueType &val, bool *not_enough_space) {
    if (!key_map_.EnoughSpace(key.size())) {
      *not_enough_space = true;
      return false;
    }

    uint16_t index = key_map_.FindLower(key, key_less);
    if (index < key_map_.Size() && key_equal(key_less, key_map_.KeyAt(index), key)) {
      *not_enough_space = false;
      return false;
    }
    key_map_.InsertKeyValue(index, key, val);
    return true;
  }

 private:
  Node *prev_;
  Node *next_;
  KeyMap<KeyType, ValueType, BPLUSTREE_LEAFNODE_SIZE - POINTER_SIZE * 2> key_map_;
};

}  // namespace pidan