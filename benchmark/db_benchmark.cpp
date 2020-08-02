#include <benchmark/benchmark.h>

#include <iostream>
#include <random>
#include <vector>

#include "common/scoped_timer.h"
#include "common/thread_pool.h"
#include "pidan/db.h"
#include "test/test_util.h"

class DBBenchmark : public benchmark::Fixture {
 public:
  void SetUp(const benchmark::State &state) final {
    for (int i = 10000000; i <= num_keys_ + 10000000; i++) {
      keys_.push_back(std::to_string(i) + std::to_string(i + 1) + std::to_string(i + 2) + std::to_string(i + 3));
    }
    std::random_device rd;
    std::mt19937 g(rd());

    std::shuffle(keys_.begin(), keys_.end(), g);
  }

  void TearDown(const benchmark::State &state) final {}
  const uint32_t num_keys_ = 10000000;
  std::vector<std::string> keys_;
};

BENCHMARK_DEFINE_F(DBBenchmark, Put)(benchmark::State &state) {
  pidan::PidanDB *db = nullptr;
  pidan::PidanDB::Open("test.db", &db);

  for (auto _ : state) {
    for (auto &i : keys_) {
      db->Put(i, "12345678123456781234567812345678");
    }
  }
}

BENCHMARK_DEFINE_F(DBBenchmark, Get)(benchmark::State &state) {
  pidan::PidanDB *db = nullptr;
  pidan::PidanDB::Open("test.db", &db);

  for (auto &i : keys_) {
    db->Put(i, "12345678123456781234567812345678");
  }

  std::string val;
  for (auto _ : state) {
    for (auto &i : keys_) {
      db->Get(i, &val);
    }
  }
}

BENCHMARK_DEFINE_F(DBBenchmark, MultiThreadGet)(benchmark::State &state) {
  int thread_num = 16;
  pidan::PidanDB *db = nullptr;
  pidan::PidanDB::Open("test.db", &db);

  for (auto &i : keys_) {
    db->Put(i, "12345678123456781234567812345678");
  }

  pidan::ThreadPool tp(thread_num);

  auto task = [&](int thread_id) {
    int start = (num_keys_ / thread_num) * thread_id;
    int end = start + num_keys_ / thread_num;
    std::string val;
    uint64_t elapsed_ms;
    {
      pidan::ScopedTimer<std::chrono::milliseconds> timer(&elapsed_ms);
      for (int i = start; i < end; i++) {
        db->Get(keys_[i], &val);
      }
    }
    std::cerr << "thread id : " << thread_id << " Get " << end - start
              << " keys use time (ms) : " << static_cast<double>(elapsed_ms) << '\n';
  };

  for (auto _ : state) {
    pidan::ThreadPoolRunWorkloadUntilFinish(&tp, task);
  }
}

BENCHMARK_DEFINE_F(DBBenchmark, MultiThreadPut)(benchmark::State &state) {
  int thread_num = 16;
  pidan::PidanDB *db = nullptr;
  for (auto _ : state) {
    pidan::PidanDB::Open("test.db", &db);

    pidan::ThreadPool tp(thread_num);

    auto task = [&](int thread_id) {
      int start = (num_keys_ / thread_num) * thread_id;
      int end = start + num_keys_ / thread_num;
      std::string val;
      uint64_t elapsed_ms;
      {
        pidan::ScopedTimer<std::chrono::milliseconds> timer(&elapsed_ms);
        for (int i = start; i < end; i++) {
          db->Put(keys_[i], "12345678123456781234567812345678");
        }
      }
      std::cerr << "thread id : " << thread_id << " Put " << end - start
                << " keys use time (ms) : " << static_cast<double>(elapsed_ms) << '\n';
    };

    pidan::ThreadPoolRunWorkloadUntilFinish(&tp, task);
    delete db;
  }
}

// BENCHMARK_REGISTER_F(DBBenchmark, Put)->Unit(benchmark::kMillisecond);
// BENCHMARK_REGISTER_F(DBBenchmark, Get)->Unit(benchmark::kMillisecond);
BENCHMARK_REGISTER_F(DBBenchmark, MultiThreadGet)->Unit(benchmark::kMillisecond);
BENCHMARK_REGISTER_F(DBBenchmark, MultiThreadPut)->Unit(benchmark::kMillisecond);

BENCHMARK_MAIN();
