#include "transaction/timestamp_manager.h"

#include <gtest/gtest.h>

#include <thread>

namespace pidan {

void TransactionThread(TimestampManager &tm) {
  timestamp_t ts = tm.BeginTransaction();
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  tm.EndTransaction();
}

TEST(TimestampManagerTest, OldestTimestamp) {
  TimestampManager tm;

  // 3个子线程分别开启一个事务
  std::thread t1(TransactionThread, std::ref(tm));
  std::thread t2(TransactionThread, std::ref(tm));
  std::thread t3(TransactionThread, std::ref(tm));

  // 我们休眠一会让其他线程有机会执行
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  timestamp_t start_ts = tm.CurrentTime();
  // 现在只有子线程开启了一个事务，所以它是最老的。
  ASSERT_EQ(start_ts, 1);
  ASSERT_EQ(tm.OldestTimestamp(), start_ts);

  // 将全局时间戳+1后主线程再开启一个事务
  tm.CheckOutTimestamp();
  ASSERT_EQ(tm.BeginTransaction(), start_ts + 1);

  t1.join();
  t2.join();
  t3.join();

  // 当子线程事务都结束时，主线程事务成为了最老的事务
  ASSERT_EQ(tm.OldestTimestamp(), start_ts + 1);
}

}  // namespace pidan