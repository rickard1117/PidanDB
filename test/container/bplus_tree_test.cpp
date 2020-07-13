#include <gtest/gtest.h>

#include <string>

#include "container/bplustree/node.h"
#include "container/bplustree/tree.h"

namespace pidan {

using Key = std::string;
using KeyComparator = std::less<Key>;

TEST(BPlusTreeNodeTest, InnerNodeAndFind) {
  Node *left = reinterpret_cast<Node *>(1);
  Node *right = reinterpret_cast<Node *>(2);
  const KeyComparator comparator;
  Key key = "bbb";
  InnerNode<Key> node(1, left, right, key);
  ASSERT_EQ(node.FindChild("aaa", comparator), left);
  ASSERT_EQ(node.FindChild("ccc", comparator), right);
}

}  // namespace pidan