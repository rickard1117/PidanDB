#include <gtest/gtest.h>

#include <string>

#include "container/bplustree/node.h"
#include "container/bplustree/tree.h"

namespace pidan {

using Key = std::string;
using Value = uint64_t;
using KeyComparator = std::less<Key>;

TEST(BPlusTreeNodeTest, InnerNodeInsertAndFind) {
  const KeyComparator comparator;
  Key key = "2";
  InnerNode<Key> node(1, reinterpret_cast<Node *>(1), reinterpret_cast<Node *>(2), key);
  ASSERT_EQ(node.FindChild("1", comparator), reinterpret_cast<Node *>(1));
  ASSERT_EQ(node.FindChild("3", comparator), reinterpret_cast<Node *>(2));

  ASSERT_TRUE(node.Insert("4", reinterpret_cast<Node *>(3), comparator));
  ASSERT_EQ(node.FindChild("1", comparator), reinterpret_cast<Node *>(1));
  ASSERT_EQ(node.FindChild("2", comparator), reinterpret_cast<Node *>(1));
  ASSERT_EQ(node.FindChild("3", comparator), reinterpret_cast<Node *>(2));
  ASSERT_EQ(node.FindChild("4", comparator), reinterpret_cast<Node *>(2));
  ASSERT_EQ(node.FindChild("5", comparator), reinterpret_cast<Node *>(3));

  ASSERT_TRUE(node.Insert("3", reinterpret_cast<Node *>(4), comparator));
  ASSERT_EQ(node.FindChild("1", comparator), reinterpret_cast<Node *>(1));
  ASSERT_EQ(node.FindChild("2", comparator), reinterpret_cast<Node *>(1));
  ASSERT_EQ(node.FindChild("3", comparator), reinterpret_cast<Node *>(2));
  ASSERT_EQ(node.FindChild("4", comparator), reinterpret_cast<Node *>(4));
  ASSERT_EQ(node.FindChild("5", comparator), reinterpret_cast<Node *>(3));

  ASSERT_TRUE(node.Insert("1", reinterpret_cast<Node *>(5), comparator));
  ASSERT_EQ(node.FindChild("0", comparator), reinterpret_cast<Node *>(1));
  ASSERT_EQ(node.FindChild("1", comparator), reinterpret_cast<Node *>(1));
  ASSERT_EQ(node.FindChild("2", comparator), reinterpret_cast<Node *>(5));
  ASSERT_EQ(node.FindChild("3", comparator), reinterpret_cast<Node *>(2));
  ASSERT_EQ(node.FindChild("4", comparator), reinterpret_cast<Node *>(4));
  ASSERT_EQ(node.FindChild("5", comparator), reinterpret_cast<Node *>(3));

  ASSERT_TRUE(node.Insert("5", reinterpret_cast<Node *>(6), comparator));
  ASSERT_EQ(node.FindChild("0", comparator), reinterpret_cast<Node *>(1));
  ASSERT_EQ(node.FindChild("1", comparator), reinterpret_cast<Node *>(1));
  ASSERT_EQ(node.FindChild("2", comparator), reinterpret_cast<Node *>(5));
  ASSERT_EQ(node.FindChild("3", comparator), reinterpret_cast<Node *>(2));
  ASSERT_EQ(node.FindChild("4", comparator), reinterpret_cast<Node *>(4));
  ASSERT_EQ(node.FindChild("5", comparator), reinterpret_cast<Node *>(3));
  ASSERT_EQ(node.FindChild("6", comparator), reinterpret_cast<Node *>(6));
}

TEST(BPlusTreeNodeTest, LeafNodeInsertAndFind) {
  const KeyComparator comparator;
  LeafNode<Key, Value> node;
  Value val = 0;
  ASSERT_FALSE(node.FindValue("1", comparator, &val));
  bool not_enough_space = false;
  ASSERT_TRUE(node.InsertUnique("1", comparator, 1, &not_enough_space));
  ASSERT_FALSE(not_enough_space);
  ASSERT_FALSE(node.InsertUnique("1", comparator, 1, &not_enough_space));
  ASSERT_FALSE(not_enough_space);
  ASSERT_TRUE(node.InsertUnique("2", comparator, 2, &not_enough_space));
  ASSERT_FALSE(not_enough_space);
}

TEST(BPlusTreeNodeTest, LeafNodeSplit) {}

TEST(BPlusTreeNodeTest, OneNodeBPTree) {
  // 测试最简单的场景，B+树中只有一个根节点。
  BPlusTree<Key, Value> tree;
  Value temp_val;

  for (int v = 1; v < 100; ++v) {
    std::string k = std::to_string(v);
    ASSERT_FALSE(tree.Lookup(k, &temp_val));
    ASSERT_TRUE(tree.InsertUnique(k, v));
    ASSERT_TRUE(tree.Lookup(k, &temp_val));
    ASSERT_EQ(temp_val, v);
  }
}

}  // namespace pidan