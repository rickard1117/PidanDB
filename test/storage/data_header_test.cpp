#include "storage/data_header.h"

#include <gtest/gtest.h>

namespace pidan {

TEST(DataHeaderTest, SingleThreadWriteConflict) {
  Transaction txn1(TransactionType::WRITE, 1, 1);
  Transaction txn2(TransactionType::WRITE, 1, 2);
  DataHeader header;

  // txn1 写入值后，txn2不可以再写入或者读取。
  ASSERT_TRUE(header.Put(&txn1, "1"));
  ASSERT_FALSE(header.Put(&txn2, "2"));
  std::string val;
  ASSERT_TRUE(header.Select(&txn1, &val));
  ASSERT_EQ(val, "1");
  ASSERT_FALSE(header.Select(&txn2, &val));
  ASSERT_EQ(val, "1");

  // txn1再次写入一个新的版本，再次读取会读到新版本；
  ASSERT_TRUE(header.Put(&txn1, "3"));
  ASSERT_TRUE(header.Select(&txn1, &val));
  ASSERT_EQ(val, "3");
}

}  // namespace pidan
