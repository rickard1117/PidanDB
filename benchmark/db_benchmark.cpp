#include <benchmark/benchmark.h>

#include <random>
#include <vector>

#include "pidan/db.h"

class DBBenchmark : public benchmark::Fixture {
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

BENCHMARK_DEFINE_F(DBBenchmark, Put)(benchmark::State &state) {
  pidan::PidanDB *db = nullptr;
  pidan::PidanDB::Open("test.db", &db);

  for (auto _ : state) {
    for (auto &i : keys_) {
      db->Put(i, "123");
    }
  }
}

BENCHMARK_REGISTER_F(DBBenchmark, Put)->Unit(benchmark::kMillisecond);

BENCHMARK_MAIN();
