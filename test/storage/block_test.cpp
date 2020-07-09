#include "storage/block.h"

#include <gtest/gtest.h>

#include "test/test_util.h"

namespace pidan {

class BlockTest : public ::testing::Test {
 protected:
  void SetUp() override { block_ = new Block(); }

  void TearDown() override { delete block_; }

  Block *block_;
};

TEST_F(BlockTest, InitSizeZero) { ASSERT_EQ(block_->ValueCount(), 0); }

TEST_F(BlockTest, Insert) {
  std::string value = "1";

  for (int i = 1; i < 100; i++) {
    value = value + std::to_string(i);
    ValueSlot v = block_->Insert(nullptr, value.data(), value.size());
    ASSERT_TRUE(v.IsValid());
    ASSERT_EQ(v.GetBlock(), block_);
    ASSERT_EQ(block_->ValueCount(), i);
  }
}

// 测试场景：随机插入一系列字符串，然后将其读取出来，检查读取结果是否正确。
TEST_F(BlockTest, Find) {
  std::vector<std::string> values;
  for (int i = 0; i < 10; i++) {
    auto random_val = test::GenRandomString(1, 100);
    values.push_back(random_val);
  }

  for (auto &val : values) {
    ValueSlot val_slot = block_->Insert(nullptr, val.data(), val.size());
    ASSERT_TRUE(val_slot.IsValid());
    auto find_val = block_->Select(nullptr, val_slot);
    ASSERT_EQ(val.compare(find_val), 0);
  }
  ASSERT_EQ(block_->ValueCount(), 10);
}

TEST_F(BlockTest, BlockFull) {
  ValueSlot vs = block_->Insert(nullptr, reinterpret_cast<const char *>(block_), BLOCK_SIZE);
  ASSERT_FALSE(vs.IsValid());

  // 插入几个随机字符串
  for (int i = 0; i < 10; i++) {
    auto random_val = test::GenRandomString(1, 100);
    auto slot = block_->Insert(nullptr, random_val.data(), random_val.size());
    ASSERT_TRUE(slot.IsValid());
  }

  vs = block_->Insert(nullptr, reinterpret_cast<const char *>(block_), block_->GetFreeSpaceRemaining());
  ASSERT_FALSE(vs.IsValid());
}

}  // namespace pidan
