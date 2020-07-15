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
  bool not_enough_space = false;
  ASSERT_FALSE(node.FindValue("1", comparator, &val));

  ASSERT_TRUE(node.InsertUnique("1", comparator, 1, &not_enough_space));
  ASSERT_TRUE(node.FindValue("1", comparator, &val));
  ASSERT_EQ(val, 1);
  ASSERT_FALSE(not_enough_space);

  ASSERT_FALSE(node.InsertUnique("1", comparator, 1, &not_enough_space));
  ASSERT_FALSE(not_enough_space);

  ASSERT_TRUE(node.InsertUnique("2", comparator, 2, &not_enough_space));
  ASSERT_TRUE(node.FindValue("2", comparator, &val));
  ASSERT_EQ(val, 2);
  ASSERT_FALSE(not_enough_space);
}

TEST(BPlusTreeNodeTest, LeafNodeSplitSimple) {
  const KeyComparator comparator;
  LeafNode<Key, Value> leaf;

  bool not_enough_space = false;
  ASSERT_TRUE(leaf.InsertUnique("1", comparator, 1, &not_enough_space));
  ASSERT_TRUE(leaf.InsertUnique("2", comparator, 2, &not_enough_space));
  ASSERT_TRUE(leaf.InsertUnique("3", comparator, 3, &not_enough_space));

  LeafNode<Key, Value> *sibling = leaf.Split();
  // 1和2留在原来的节点中，3被分裂出去。
  ASSERT_EQ(leaf.size(), 2);
  ASSERT_EQ(sibling->size(), 1);

  Value temp_val = 0;
  ASSERT_TRUE(leaf.FindValue("1", comparator, &temp_val));
  ASSERT_EQ(temp_val, 1);

  ASSERT_TRUE(leaf.FindValue("2", comparator, &temp_val));
  ASSERT_EQ(temp_val, 2);

  ASSERT_FALSE(leaf.FindValue("3", comparator, &temp_val));

  ASSERT_TRUE(sibling->FindValue("3", comparator, &temp_val));
  ASSERT_EQ(temp_val, 3);
}

TEST(BPlusTreeNodeTest, LeafNodeSplit) {
  const KeyComparator comparator;
  LeafNode<Key, Value> leaf;

  bool not_enough_space = false;
  Value temp_val = 0;
  for (int i = 100; i <= 299; i++) {
    std::string key = std::to_string(i);
    ASSERT_TRUE(leaf.InsertUnique(key, comparator, i, &not_enough_space));
    ASSERT_TRUE(leaf.FindValue(key, comparator, &temp_val));
    ASSERT_EQ(temp_val, i);
  }

  LeafNode<Key, Value> *sibling = leaf.Split();

  int left_size = leaf.size();
  int rigght_size = sibling->size();

  ASSERT_EQ(200, left_size + rigght_size);

  for (int i = 100; i <= left_size; i++) {
    std::string key = std::to_string(i);
    std::cerr << "going to find key : " << key << '\n';
    ASSERT_TRUE(leaf.FindValue(key, comparator, &temp_val));
    ASSERT_EQ(temp_val, i);
  }
  for (int i = 100 + left_size + 1; i <= 299; i++) {
    std::string key = std::to_string(i);
    ASSERT_TRUE(sibling->FindValue(key, comparator, &temp_val));
    ASSERT_EQ(temp_val, i);
  }
}

TEST(BPlusTreeTest, OneNodeBPTree) {
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