#include "transaction/transaction_manager.h"

#include <gtest/gtest.h>

#include "storage/data_header.h"
#include "transaction/transaction.h"

namespace pidan {

TEST(TransactionManagerTest, MixupTest) {
  TimestampManager ts_manager;
  TransactionManager txn_manager(&ts_manager);
  DataHeader data_header1, data_header2;
  auto *txn1 = txn_manager.BeginWriteTransaction();
  auto *txn2 = txn_manager.BeginWriteTransaction();

  ASSERT_EQ(txn1->Timestamp(), txn2->Timestamp());

  // txn先写入者获胜
  ASSERT_TRUE(data_header1.Put(txn1, "abc"));
  ASSERT_FALSE(data_header1.Put(txn2, "aaa"));

  // txn1继续写入相同和不同的数据项
  ASSERT_TRUE(data_header1.Put(txn1, "def"));
  ASSERT_TRUE(data_header1.Put(txn1, "ghi"));
  ASSERT_TRUE(data_header2.Put(txn1, "123"));

  // txn2没有办法查到任何txn1写入但是还未提交的数据
  // 正常情况下，这里txn2执行Select失败是要Abort的，这里只是为了测试。
  std::string temp_val;
  bool not_found;
  ASSERT_FALSE(data_header1.Select(txn2, &temp_val, &not_found));
  ASSERT_FALSE(data_header2.Select(txn2, &temp_val, &not_found));

  // 此时txn1提交，txn2能读到txn1的改动，也可以进行写操作。
  txn_manager.Commit(txn1);
  ASSERT_TRUE(data_header1.Select(txn2, &temp_val, &not_found));
  ASSERT_FALSE(not_found);
  ASSERT_EQ(temp_val, "ghi");
  ASSERT_TRUE(data_header2.Select(txn2, &temp_val, &not_found));
  ASSERT_FALSE(not_found);
  ASSERT_EQ(temp_val, "123");
  ASSERT_TRUE(data_header1.Put(txn2, "xyz"));
  ASSERT_TRUE(data_header2.Put(txn2, "zyx"));

  // txn2 也只能看到自己的改动
  ASSERT_TRUE(data_header1.Select(txn2, &temp_val, &not_found));
  ASSERT_FALSE(not_found);
  ASSERT_EQ(temp_val, "xyz");
  ASSERT_TRUE(data_header2.Select(txn2, &temp_val, &not_found));
  ASSERT_FALSE(not_found);
  ASSERT_EQ(temp_val, "zyx");

  // 再开启一个新的读事务，但是只能读到txn1已经提交的内容。
  auto *txn3 = txn_manager.BeginReadTransaction();
  ASSERT_EQ(txn2->Timestamp(), txn3->Timestamp() - 1);
  ASSERT_TRUE(data_header1.Select(txn3, &temp_val, &not_found));
  ASSERT_FALSE(not_found);
  ASSERT_EQ(temp_val, "ghi");
  ASSERT_TRUE(data_header2.Select(txn3, &temp_val, &not_found));
  ASSERT_FALSE(not_found);
  ASSERT_EQ(temp_val, "123");
}

TEST(TransactionManagerTest, SelectNotFound) {
    // dataheader刚被一个写事务创建，此时它写入的内容不会被任何其他人看到，读事务会返回not_found

}

}  // namespace pidan