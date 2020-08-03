#include "transaction/timestamp_manager.h"

#include "common/config.h"

namespace pidan {

namespace {

// active_txn中的元素的值有三种含义
// active_txn[index] == 0 表示该下标没有被占用，即系统中没有以index为id的线程存在。
// active_txn[index] == MAX_TIMESTAMP 表示线程id为index的线程当前没有正在执行的事务。
// active_txn[index] == ts 表示线程id为index的线程当前所执行事务的启动时间为ts
std::atomic<timestamp_t> active_txn[MAX_ACCESS_THREAD];

int AssignThreadID() {
  for (int i = 0; i < MAX_ACCESS_THREAD; i++) {
    if (active_txn[i] == 0) {
      active_txn[i] = MAX_TIMESTAMP;
      return i;
    }
  }
  throw std::runtime_error("no enough threads id to assign ");
}

void ThreadEnd(int id) { active_txn[id] = 0; }

class ThreadLocalObject {
 public:
  DISALLOW_COPY_AND_MOVE(ThreadLocalObject);

  ThreadLocalObject() : id_(AssignThreadID()){};

  ~ThreadLocalObject() { ThreadEnd(id_); }

  int ThreadID() { return id_; }

 private:
  int id_;
};

}  // namespace

thread_local ThreadLocalObject local_obj;

timestamp_t TimestampManager::BeginTransaction() {
  auto ts = CurrentTime();
  active_txn[local_obj.ThreadID()] = ts;
  return ts;
}

int TimestampManager::ThreadID() { return local_obj.ThreadID(); }

void TimestampManager::EndTransaction() { active_txn[local_obj.ThreadID()] = MAX_TIMESTAMP; }

timestamp_t TimestampManager::OldestTimestamp() {
  timestamp_t result = active_txn[0].load();

  for (int i = 1; i < MAX_ACCESS_THREAD; i++) {
    timestamp_t ts = active_txn[i].load();
    if (ts != 0 && (ts < result || result == 0)) {
      result = ts;
    }
  }
  return result;
}

}  // namespace pidan