#include <benchmark/benchmark.h>

#include <iostream>
#include <random>
#include <vector>

#include "common/scoped_timer.h"
#include "common/thread_pool.h"
#include "container/bplustree/tree.h"
#include "pidan/slice.h"
#include "test/test_util.h"

using Key = pidan::Slice;
using Value = uint64_t;

class BPlusTreeBenchmark : public benchmark::Fixture {
 public:
  void SetUp(const benchmark::State &state) final {
    for (int i = 10000000; i < num_keys_ + 10000000; i++) {
      keys_.push_back(std::to_string(i) + std::to_string(i + 1) + std::to_string(i + 2));
    }
    std::random_device rd;
    std::mt19937 g(rd());

    std::shuffle(keys_.begin(), keys_.end(), g);
  }

  void TearDown(const benchmark::State &state) final {}
  const uint32_t num_keys_ = 3200000;
  std::vector<std::string> keys_;
};

BENCHMARK_DEFINE_F(BPlusTreeBenchmark, BPlusTreeLookup)(benchmark::State &state) {
  pidan::BPlusTree<Key, Value> tree;
  Value temp_val;

  for (auto &k : keys_) {
    tree.InsertUnique(k, 0, &temp_val);
  }

  for (auto _ : state) {
    for (auto &i : keys_) {
      tree.Lookup(i, &temp_val);
    }
  }
}

BENCHMARK_DEFINE_F(BPlusTreeBenchmark, BPlusTreeInsert)(benchmark::State &state) {
  pidan::BPlusTree<Key, Value> tree;
  Value temp_val;

  for (auto _ : state) {
    for (auto &k : keys_) {
      tree.InsertUnique(k, 0, &temp_val);
    }
  }
}

BENCHMARK_DEFINE_F(BPlusTreeBenchmark, BPlusTreeLookupMultiThread)(benchmark::State &state) {
  pidan::BPlusTree<Key, Value> tree;
  Value temp_val;
  int thread_num = 8;

  for (auto &k : keys_) {
    tree.InsertUnique(k, 0, &temp_val);
  }

  pidan::ThreadPool tp(thread_num);
  auto task = [&](int thread_id) {
    int start = (num_keys_ / thread_num) * thread_id;
    int end = start + num_keys_ / thread_num;
    Value val;
    uint64_t elapsed_ms;
    {
      pidan::ScopedTimer<std::chrono::milliseconds> timer(&elapsed_ms);
      for (int i = start; i < end; i++) {
        tree.Lookup(keys_[i], &val);
      }
    }
    std::cerr << "thread id : " << thread_id << " lookup " << end - start
              << " keys use time (ms) : " << static_cast<double>(elapsed_ms) << '\n';
  };

  for (auto _ : state) {
    pidan::ThreadPoolRunWorkloadUntilFinish(&tp, task);
  }
}

BENCHMARK_DEFINE_F(BPlusTreeBenchmark, BPlusTreeInsertMultiThread)(benchmark::State &state) {
  pidan::BPlusTree<Key, Value> tree;
  Value temp_val;
  int thread_num = 8;

  pidan::ThreadPool tp(thread_num);
  auto task = [&](int thread_id) {
    int start = (num_keys_ / thread_num) * thread_id;
    int end = start + num_keys_ / thread_num;
    Value val;
    uint64_t elapsed_ms;
    {
      pidan::ScopedTimer<std::chrono::milliseconds> timer(&elapsed_ms);
      for (int i = start; i < end; i++) {
        tree.InsertUnique(keys_[i], 0, &val);
      }
    }
    std::cerr << "thread id : " << thread_id << " insert " << end - start
              << " keys use time (ms) : " << static_cast<double>(elapsed_ms) << '\n';
  };

  for (auto _ : state) {
    pidan::ThreadPoolRunWorkloadUntilFinish(&tp, task);
  }
}

// BENCHMARK_REGISTER_F(BPlusTreeBenchmark, BPlusTreeLookup)->Unit(benchmark::kMillisecond);
// BENCHMARK_REGISTER_F(BPlusTreeBenchmark, BPlusTreeInsert)->Unit(benchmark::kMillisecond);
BENCHMARK_REGISTER_F(BPlusTreeBenchmark, BPlusTreeLookupMultiThread)->Unit(benchmark::kMillisecond);
// BENCHMARK_REGISTER_F(BPlusTreeBenchmark, BPlusTreeInsertMultiThread)->Unit(benchmark::kMillisecond);
BENCHMARK_MAIN();