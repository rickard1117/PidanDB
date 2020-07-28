#include <benchmark/benchmark.h>

#include <random>
#include <vector>

#include "pidan/slice.h"
#include "container/bplustree/tree.h"

using Key = pidan::Slice;
using Value = uint64_t;

class BPlusTreeBenchmark : public benchmark::Fixture {
 public:
  void SetUp(const benchmark::State &state) final {
    for (int i = 10000000; i <= num_keys_ + 10000000; i++) {
      keys_.push_back(std::to_string(i) + std::to_string(i + 1) + std::to_string(i + 2));
    }
    std::random_device rd;
    std::mt19937 g(rd());

    std::shuffle(keys_.begin(), keys_.end(), g);
  }

  void TearDown(const benchmark::State &state) final {}
  const uint32_t num_keys_ = 1000000;
  std::vector<std::string> keys_;
};

BENCHMARK_DEFINE_F(BPlusTreeBenchmark, BPlusTreeLookup)(benchmark::State &state) {
  pidan::BPlusTree<Key, Value> tree;
  Value temp_val;

  for (auto &k : keys_) {
    tree.InsertUnique(k, 0);
  }

  for (auto _ : state) {
    for (auto &i : keys_) {
      tree.Lookup(i, &temp_val);
    }
  }
}

BENCHMARK_REGISTER_F(BPlusTreeBenchmark, BPlusTreeLookup)->Unit(benchmark::kMillisecond);

BENCHMARK_MAIN();