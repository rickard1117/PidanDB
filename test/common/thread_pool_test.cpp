#include "common/thread_pool.h"

#include <gtest/gtest.h>

#include <atomic>

namespace pidan {

TEST(ThreadPoolTest, SimpleMixup) {
  ThreadPool tp(4);
  std::atomic<int> i(0);
  tp.Start();
  tp.AddTask([&] { i.fetch_add(1); });
  tp.AddTask([&] { i.fetch_add(1); });
  tp.AddTask([&] { i.fetch_add(1); });
  tp.WaitUntilAllTasksFinished();
  ASSERT_EQ(i.load(), 3);

  tp.AddTask([&] { i.fetch_add(1); });
  tp.WaitUntilAllTasksFinished();
  ASSERT_EQ(i.load(), 4);
  tp.Shutdown();

  tp.AddTask([&] { i.fetch_add(1); });
  tp.AddTask([&] { i.fetch_add(1); });
  tp.AddTask([&] { i.fetch_add(1); });

  tp.Start();
  tp.WaitUntilAllTasksFinished();
  ASSERT_EQ(i.load(), 7);
  tp.Shutdown();
}

}  // namespace pidan