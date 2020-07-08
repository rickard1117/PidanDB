#include "storage/block.h"

#include <gtest/gtest.h>

#include <iostream>

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

}  // namespace pidan
