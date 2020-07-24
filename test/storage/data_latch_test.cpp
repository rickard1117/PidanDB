#include "storage/data_latch.h"

#include <gtest/gtest.h>

#include <thread>

namespace pidan {

TEST(DataLatchTest, SingleThreadTest) {
  DataLatch latch;
  // 测试场景：加了写锁之后就不可以再加其他类型的锁
  ASSERT_TRUE(latch.TryWriteLock(1));
  ASSERT_FALSE(latch.TryWriteLock(2));
  ASSERT_TRUE(latch.TryWriteLock(1));
  ASSERT_FALSE(latch.TryReadLock());
  latch.WriteUnlock(1);

  // 测试场景：加了读锁之后不可以再加写锁，并且测试读锁的引用计数功能。
  ASSERT_TRUE(latch.TryReadLock());
  ASSERT_FALSE(latch.TryWriteLock(1));
  ASSERT_TRUE(latch.TryReadLock());
  latch.ReadUnlock();
  ASSERT_FALSE(latch.TryWriteLock(1));
  ASSERT_TRUE(latch.TryReadLock());
  latch.ReadUnlock();
  latch.ReadUnlock();
  ASSERT_TRUE(latch.TryWriteLock(1));
}

TEST(DataLatchTest, MultiThreadTest) {}

}  // namespace pidan