#pragma once

#include "common/macros.h"
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
class Node {
 public:
  MEM_REINTERPRET_CAST_ONLY(Node);
  bool IsLeaf() const { return level_ == 0; }
  uint16_t level_;
};

template <typename KeyType, typename ValueType>
class InnerNode : public Node {
 public:
};

template <typename KeyType, typename ValueType>
class LeafNode : public Node {
 public:
};
