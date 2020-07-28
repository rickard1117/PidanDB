#include <gtest/gtest.h>

#include <algorithm>
#include <iostream>
#include <iterator>
#include <map>
#include <random>
#include <set>
#include <string>

#include "container/bplustree/node.h"
#include "container/bplustree/tree.h"
#include "pidan/slice.h"
#include "test/test_util.h"

namespace pidan {

using Key = Slice;
using Value = uint64_t;
using KeyComparator = std::less<Key>;

TEST(BPlusTreeKeyMapTest, KeyMapSplit) {
  KeyMap<Key, Value, 4096> key_map;
  key_map.InsertKeyValue(key_map.FindLower("2"), "2", 2);
  key_map.InsertKeyValue(key_map.FindLower("4"), "4", 4);
  key_map.InsertKeyValue(key_map.FindLower("3"), "3", 3);
  ASSERT_EQ(key_map.KeyAt(0), "2");
  ASSERT_EQ(key_map.KeyAt(1), "3");
  ASSERT_EQ(key_map.KeyAt(2), "4");
  ASSERT_EQ(key_map.ValueAt(0), 2);
  ASSERT_EQ(key_map.ValueAt(1), 3);
  ASSERT_EQ(key_map.ValueAt(2), 4);

  KeyMap<Key, Value, 4096> new_key_map;
  auto kv = key_map.SplitWithKey(&new_key_map);
  ASSERT_EQ(kv.first, "3");
  ASSERT_EQ(kv.second, 3);

  ASSERT_EQ(key_map.size(), 1);
  ASSERT_EQ(new_key_map.size(), 1);
  ASSERT_EQ(key_map.KeyAt(0), "2");
  ASSERT_EQ(key_map.ValueAt(0), 2);

  ASSERT_EQ(new_key_map.KeyAt(0), "4");
  ASSERT_EQ(new_key_map.ValueAt(0), 4);
}

TEST(BPlusTreeNodeTest, InnerNodeInsertAndFind) {
  Key key = "2";
  InnerNode<Key> node(1, reinterpret_cast<Node *>(1), reinterpret_cast<Node *>(2), key);
  ASSERT_EQ(node.FindChild("1"), reinterpret_cast<Node *>(1));
  ASSERT_EQ(node.FindChild("3"), reinterpret_cast<Node *>(2));

  ASSERT_TRUE(node.Insert("4", reinterpret_cast<Node *>(3)));
  ASSERT_EQ(node.FindChild("1"), reinterpret_cast<Node *>(1));
  ASSERT_EQ(node.FindChild("2"), reinterpret_cast<Node *>(1));
  ASSERT_EQ(node.FindChild("3"), reinterpret_cast<Node *>(2));
  ASSERT_EQ(node.FindChild("4"), reinterpret_cast<Node *>(2));
  ASSERT_EQ(node.FindChild("5"), reinterpret_cast<Node *>(3));

  ASSERT_TRUE(node.Insert("3", reinterpret_cast<Node *>(4)));
  ASSERT_EQ(node.FindChild("1"), reinterpret_cast<Node *>(1));
  ASSERT_EQ(node.FindChild("2"), reinterpret_cast<Node *>(1));
  ASSERT_EQ(node.FindChild("3"), reinterpret_cast<Node *>(2));
  ASSERT_EQ(node.FindChild("4"), reinterpret_cast<Node *>(4));
  ASSERT_EQ(node.FindChild("5"), reinterpret_cast<Node *>(3));

  ASSERT_TRUE(node.Insert("1", reinterpret_cast<Node *>(5)));
  ASSERT_EQ(node.FindChild("0"), reinterpret_cast<Node *>(1));
  ASSERT_EQ(node.FindChild("1"), reinterpret_cast<Node *>(1));
  ASSERT_EQ(node.FindChild("2"), reinterpret_cast<Node *>(5));
  ASSERT_EQ(node.FindChild("3"), reinterpret_cast<Node *>(2));
  ASSERT_EQ(node.FindChild("4"), reinterpret_cast<Node *>(4));
  ASSERT_EQ(node.FindChild("5"), reinterpret_cast<Node *>(3));

  ASSERT_TRUE(node.Insert("5", reinterpret_cast<Node *>(6)));
  ASSERT_EQ(node.FindChild("0"), reinterpret_cast<Node *>(1));
  ASSERT_EQ(node.FindChild("1"), reinterpret_cast<Node *>(1));
  ASSERT_EQ(node.FindChild("2"), reinterpret_cast<Node *>(5));
  ASSERT_EQ(node.FindChild("3"), reinterpret_cast<Node *>(2));
  ASSERT_EQ(node.FindChild("4"), reinterpret_cast<Node *>(4));
  ASSERT_EQ(node.FindChild("5"), reinterpret_cast<Node *>(3));
  ASSERT_EQ(node.FindChild("6"), reinterpret_cast<Node *>(6));
}

TEST(BPlusTreeNodeTest, LeafNodeInsertAndFind) {
  LeafNode<Key, Value> node;
  Value val = 0;
  bool not_enough_space = false;
  ASSERT_FALSE(node.FindValue("1", &val));
  ASSERT_TRUE(node.InsertUnique("1", 1, &not_enough_space));
  ASSERT_TRUE(node.FindValue("1", &val));
  ASSERT_EQ(val, 1);
  ASSERT_FALSE(not_enough_space);

  ASSERT_FALSE(node.InsertUnique("1", 1, &not_enough_space));
  ASSERT_FALSE(not_enough_space);

  ASSERT_TRUE(node.InsertUnique("2", 2, &not_enough_space));
  ASSERT_TRUE(node.FindValue("2", &val));
  ASSERT_EQ(val, 2);
  ASSERT_FALSE(not_enough_space);
}

TEST(BPlusTreeNodeTest, LeafNodeSplitSimple) {
  LeafNode<Key, Value> leaf;

  bool not_enough_space = false;
  ASSERT_TRUE(leaf.InsertUnique("1", 1, &not_enough_space));
  ASSERT_TRUE(leaf.InsertUnique("2", 2, &not_enough_space));
  ASSERT_TRUE(leaf.InsertUnique("3", 3, &not_enough_space));

  LeafNode<Key, Value> *sibling = leaf.Split();
  // 1和2留在原来的节点中，3被分裂出去。
  ASSERT_EQ(leaf.size(), 2);
  ASSERT_EQ(sibling->size(), 1);

  Value temp_val = 0;
  ASSERT_TRUE(leaf.FindValue("1", &temp_val));
  ASSERT_EQ(temp_val, 1);

  ASSERT_TRUE(leaf.FindValue("2", &temp_val));
  ASSERT_EQ(temp_val, 2);

  ASSERT_FALSE(leaf.FindValue("3", &temp_val));

  ASSERT_TRUE(sibling->FindValue("3", &temp_val));
  ASSERT_EQ(temp_val, 3);
}

TEST(BPlusTreeNodeTest, LeafNodeSplit) {
  LeafNode<Key, Value> leaf;

  bool not_enough_space = false;
  Value temp_val = 0;
  for (int i = 100; i <= 299; i++) {
    std::string key = std::to_string(i);
    ASSERT_TRUE(leaf.InsertUnique(key, i, &not_enough_space));
    ASSERT_TRUE(leaf.FindValue(key, &temp_val));
    ASSERT_EQ(temp_val, i);
  }

  LeafNode<Key, Value> *sibling = leaf.Split();

  int left_size = leaf.size();
  int rigght_size = sibling->size();

  ASSERT_EQ(200, left_size + rigght_size);

  for (int i = 100; i <= left_size; i++) {
    std::string key = std::to_string(i);
    ASSERT_TRUE(leaf.FindValue(key, &temp_val));
    ASSERT_EQ(temp_val, i);
  }
  for (int i = 100 + left_size + 1; i <= 299; i++) {
    std::string key = std::to_string(i);
    ASSERT_TRUE(sibling->FindValue(key, &temp_val));
    ASSERT_EQ(temp_val, i);
  }
}

TEST(BPlusTreeNodeTest, InnerNodeSplitSimple) {
  Key key = "2";
  InnerNode<Key> node(1, reinterpret_cast<Node *>(1), reinterpret_cast<Node *>(2), key);
  ASSERT_TRUE(node.Insert("4", reinterpret_cast<Node *>(3)));
  ASSERT_TRUE(node.Insert("3", reinterpret_cast<Node *>(4)));
  Key split_key;
  auto sibling = node.Split(&split_key);
  EXPECT_EQ(node.size(), 1);
  EXPECT_EQ(sibling->size(), 1);
  ASSERT_EQ(split_key, "3");
  ASSERT_EQ(node.FindChild("1"), reinterpret_cast<Node *>(1));
  ASSERT_EQ(node.FindChild("2"), reinterpret_cast<Node *>(1));
  ASSERT_EQ(node.FindChild("3"), reinterpret_cast<Node *>(2));
  ASSERT_EQ(node.FindChild("4"), reinterpret_cast<Node *>(2));
  ASSERT_EQ(node.FindChild("5"), reinterpret_cast<Node *>(2));

  ASSERT_EQ(sibling->FindChild("3"), reinterpret_cast<Node *>(4));
  ASSERT_EQ(sibling->FindChild("4"), reinterpret_cast<Node *>(4));
  ASSERT_EQ(sibling->FindChild("5"), reinterpret_cast<Node *>(3));
  ASSERT_EQ(sibling->FindChild("6"), reinterpret_cast<Node *>(3));
}

TEST(BPlusTreeNodeTest, InnerNodeSplit) {
  Key key = "100";
  InnerNode<Key> node(1, reinterpret_cast<Node *>(0), reinterpret_cast<Node *>(100), key);

  std::vector<int> keys;
  for (int i = 101; i <= 199; i++) {
    keys.push_back(i);
  }
  std::random_device rd;
  std::mt19937 g(rd());

  std::shuffle(keys.begin(), keys.end(), g);

  for (auto key : keys) {
    ASSERT_TRUE(node.Insert(std::to_string(key), reinterpret_cast<Node *>(key)));
  }

  Key split_key;
  auto sibling = node.Split(&split_key);
  EXPECT_EQ(node.size(), 49);
  EXPECT_EQ(sibling->size(), 50);
  ASSERT_EQ(split_key, "149");

  ASSERT_EQ(node.FindChild("100"), reinterpret_cast<Node *>(0));
  for (int i = 101; i < 150; i++) {
    ASSERT_EQ(node.FindChild(std::to_string(i)), reinterpret_cast<Node *>(i - 1));
  }
  ASSERT_EQ(node.FindChild(std::to_string(150)), reinterpret_cast<Node *>(148));
  for (int i = 150; i < 199; i++) {
    ASSERT_EQ(sibling->FindChild(std::to_string(i)), reinterpret_cast<Node *>(i - 1));
  }
}

TEST(BPlusTreeNodeTest, InnerNodeNotEnoughSpace) {
  // 测试场景：将随机字符串插入inner node中直到空间不足
  // 然后分裂节点，检查分裂后两个节点内容的正确性。
  std::map<std::string, Node *> keys;
  int fake_child = 1;
  keys["abcdefghijkl"] = reinterpret_cast<Node *>(fake_child++);
  InnerNode<Key> node(1, nullptr, keys["abcdefghijkl"], "abcdefghijkl");
  for (;;) {
    std::string s = ::pidan::test::GenRandomString(10, 100);
    if (!node.EnoughSpaceFor(s.size())) {
      ASSERT_FALSE(node.Insert(s, reinterpret_cast<Node *>(fake_child)));
      break;
    }
    ASSERT_TRUE(node.Insert(s, reinterpret_cast<Node *>(fake_child)));
    keys[s] = reinterpret_cast<Node *>(fake_child++);
  }

  Slice split_key;
  auto sibling = node.Split(&split_key);
  ASSERT_EQ(keys.size() - 1, node.size() + sibling->size());
}

TEST(BPlusTreeTest, RandomInteger) {
  BPlusTree<Key, Value> tree;
  Value temp_val;

  std::vector<int> keys;
  for (int i = 0; i <= 99999; i++) {
    keys.push_back(i);
  }
  std::random_device rd;
  std::mt19937 g(rd());

  std::shuffle(keys.begin(), keys.end(), g);

  for (auto &v : keys) {
    std::string k = std::to_string(v);
    ASSERT_FALSE(tree.Lookup(k, &temp_val));
    ASSERT_TRUE(tree.InsertUnique(k, v));
    ASSERT_TRUE(tree.Lookup(k, &temp_val));
    ASSERT_EQ(temp_val, v);
  }
}

// TEST(BPlusTreeTest, RandomString) {
//   std::set<std::string> kvs;
//   BPlusTree<Key, Value> tree;
//   Value temp_val;

//   // std::set<std::string> keys;
//   for (int i = 0; i < 1024; i++) {
//   // for (int i = 1000; i < 3000; i++) {
//     // std::cerr << "loop : " << i << '\n';
//     std::string k = ::pidan::test::GenRandomString(8, 128);
//     if (kvs.find(k) != kvs.end()) {
//       std::cerr << "fuck !" << '\n';
//     } else {
//       kvs.insert(k);
//     }
//     // std::string k = std::to_string(i);
//     if (!tree.Lookup(k, &temp_val)) {
//       tree.DrawTreeDot("RandomInsertAndLookupTree.dot");
//     }
//     ASSERT_FALSE(tree.Lookup(k, &temp_val));
//     ASSERT_TRUE(tree.InsertUnique(k, i));
//     ASSERT_TRUE(tree.Lookup(k, &temp_val));
//     ASSERT_EQ(temp_val, i);
//   }
// }

}  // namespace pidan