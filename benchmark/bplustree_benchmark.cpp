#include <benchmark/benchmark.h>

#include <random>
#include <vector>

#include "common/slice.h"
#include "container/bplustree/tree.h"

using Key = pidan::Slice;
using Value = uint64_t;

static void BM_BPlusTreeLookup(benchmark::State &state) {
  pidan::BPlusTree<Key, Value> tree;
  Value temp_val;

  std::vector<std::string> keys;
  for (int i = 0; i <= 100000; i++) {
    keys.push_back(std::to_string(i));
  }
  std::random_device rd;
  std::mt19937 g(rd());

  std::shuffle(keys.begin(), keys.end(), g);
  for (auto &k : keys) {
    tree.InsertUnique(k, 0);
  }

  for (auto _ : state) {
    for (auto &i : keys) {
      tree.Lookup(i, &temp_val);
    }
  }
}
BENCHMARK(BM_BPlusTreeLookup);

BENCHMARK_MAIN();