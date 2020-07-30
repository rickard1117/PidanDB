#pragma once
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>

#include "common/macros.h"

namespace pidan {

class ThreadPool {
 public:
  using Task = std::function<void(void)>;
  using TaskQueue = std::queue<Task>;

  DISALLOW_COPY_AND_MOVE(ThreadPool);

  ThreadPool(int thread_num, TaskQueue queue) : tasks_(std::move(queue)), thread_num_(thread_num) {}

  ThreadPool(int thread_num) : thread_num_(thread_num) {}

  // 启动线程池
  void Start() {
    {
      const std::lock_guard<std::mutex> lock(tasks_lock_);
      terminate_ = false;
    }

    for (int i = 0; i < thread_num_; i++) {
      AddThread();
    }
  }

  // 关闭线程池，每个线程执行完当前任务后会直接退出。
  void Shutdown() {
    {
      const std::lock_guard<std::mutex> lock(tasks_lock_);
      terminate_ = true;
    }
    tasks_cv_.notify_all();
    for (auto &worker : workers_) {
      worker.join();
    }
    workers_.clear();
  }

  void AddTask(Task task) {
    {
      const std::lock_guard<std::mutex> lock(tasks_lock_);
      tasks_.emplace(std::move(task));
    }
    tasks_cv_.notify_one();
  }

  // 阻塞等待，直到队列中所有任务完成
  void WaitUntilAllTasksFinished() {
    std::unique_lock<std::mutex> lock(tasks_lock_);
    finished_cv_.wait(lock, [&] { return busy_worker_ == 0 && tasks_.empty(); });
  }

  int ThreadNum() const { return workers_.size(); }

 private:
  void AddThread() {
    workers_.emplace_back([this] {
      Task task;
      for (;;) {
        {
          std::unique_lock<std::mutex> lock(tasks_lock_);
          tasks_cv_.wait(lock, [&] { return terminate_ || !tasks_.empty(); });
          // 这里还是加锁的
          if (terminate_) {
            return;
          }
          task = std::move(tasks_.front());
          tasks_.pop();
          ++busy_worker_;
        }

        task();
        {
          const std::lock_guard<std::mutex> lock(tasks_lock_);
          --busy_worker_;
        }

        finished_cv_.notify_one();
      }
    });
  }

  std::vector<std::thread> workers_;
  std::mutex tasks_lock_;
  std::queue<Task> tasks_;            // 被tasks_lock_保护
  std::condition_variable tasks_cv_;  // 被tasks_lock_保护
  std::condition_variable finished_cv_;
  bool terminate_{true};  // 被tasks_lock_保护
  int busy_worker_{0};    // 被tasks_lock_保护
  int thread_num_{0};
};

}  // namespace pidan