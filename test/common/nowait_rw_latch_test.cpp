#include "common/nowait_rw_latch.h"

#include <gtest/gtest.h>

#include <chrono>
#include <thread>

namespace pidan {

TEST(NoWaitRWLatchTest, SingleThreadTest) {
  NoWaitRWLatch latch;

  // 写锁和读锁冲突
  ASSERT_TRUE(latch.TryWriteLock());
  ASSERT_FALSE(latch.TryWriteLock());
  ASSERT_FALSE(latch.TryReadLock());
  latch.WriteUnlock();

  // 读锁和写锁冲突
  ASSERT_TRUE(latch.TryReadLock());
  ASSERT_FALSE(latch.TryWriteLock());
  latch.ReadUnlock();

  // 读锁之间不冲突
  ASSERT_TRUE(latch.TryReadLock());
  ASSERT_TRUE(latch.TryReadLock());
  latch.ReadUnlock();
  latch.ReadUnlock();

  ASSERT_TRUE(latch.NoLock());
}

void LongWrite(NoWaitRWLatch &latch, int &flag) {
  if (!latch.TryWriteLock()) {
    return;
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  flag++;
  latch.WriteUnlock();
}

TEST(NoWaitRWLatchTest, MultiThreadWrite) {
  // 多个线程同时加写锁，只有一个能成功
  NoWaitRWLatch latch;
  int flag = 0;
  std::thread t1(LongWrite, std::ref(latch), std::ref(flag)), t2(LongWrite, std::ref(latch), std::ref(flag)),
      t3(LongWrite, std::ref(latch), std::ref(flag));

  t1.join();
  t2.join();
  t3.join();
  ASSERT_EQ(flag, 1);
  ASSERT_TRUE(latch.NoLock());
}

void Read(NoWaitRWLatch &latch) {
  ASSERT_TRUE(latch.TryReadLock());
  latch.ReadUnlock();
}

TEST(NoWaitRWLatchTest, MultiThreadRead) {
  NoWaitRWLatch latch;
  std::thread t1(Read, std::ref(latch)), t2(Read, std::ref(latch)), t3(Read, std::ref(latch));
  t1.join();
  t2.join();
  t3.join();
  ASSERT_TRUE(latch.NoLock());
}

}  // namespace pidan
