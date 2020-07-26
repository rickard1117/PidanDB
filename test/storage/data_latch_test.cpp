#include "storage/data_latch.h"

#include <gtest/gtest.h>

#include <chrono>
#include <thread>

namespace pidan {

TEST(DataLatchTest, SingleThreadTest) {
  DataLatch latch;
  // 测试场景：加了写锁之后就不可以再加其他类型的锁
  ASSERT_TRUE(latch.TryWriteLock(1));
  ASSERT_FALSE(latch.TryWriteLock(2));
  ASSERT_TRUE(latch.TryWriteLock(1));
  ASSERT_FALSE(latch.TryReadLock(2));
  latch.WriteUnlock(1);

  // 测试场景：加了读锁之后不可以再加写锁，并且测试读锁的引用计数功能。
  ASSERT_TRUE(latch.TryReadLock(2));
  ASSERT_FALSE(latch.TryWriteLock(1));
  ASSERT_TRUE(latch.TryReadLock(2));
  latch.ReadUnlock();
  ASSERT_FALSE(latch.TryWriteLock(1));
  ASSERT_TRUE(latch.TryReadLock(2));
  latch.ReadUnlock();
  latch.ReadUnlock();
  ASSERT_TRUE(latch.TryWriteLock(1));
}

void Read(DataLatch &latch) { 
  EXPECT_TRUE(latch.TryReadLock(1)); 
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  latch.ReadUnlock();
}

void Write(DataLatch &latch) {
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  EXPECT_FALSE(latch.TryWriteLock(1));
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  EXPECT_TRUE(latch.TryWriteLock(1));
  latch.WriteUnlock(1);
}

TEST(DataLatchTest, MultiThreadTest) {
  DataLatch latch;
  std::thread t1(Read, std::ref(latch)), t2(Write, std::ref(latch));

  t1.join();
  t2.join();
}



}  // namespace pidan