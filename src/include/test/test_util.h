#pragma once

#include <algorithm>
#include <chrono>
#include <climits>
#include <functional>
#include <random>
#include <string>

namespace pidan::test {

std::string GenRandomString(size_t min_size, size_t max_size) {
  std::random_device rd;   // Will be used to obtain a seed for the random number engine
  std::mt19937 gen(rd());  // Standard mersenne_twister_engine seeded with rd()
  std::uniform_int_distribution<size_t> distrib(min_size, max_size);
  using random_bytes_engine = std::independent_bits_engine<std::default_random_engine, CHAR_BIT, unsigned char>;

  random_bytes_engine rbe;
  rbe.seed(std::chrono::system_clock::now().time_since_epoch().count());
  auto random = distrib(gen);

  std::string data;
  data.resize(random);
  std::generate(begin(data), end(data), rbe);
  return data;
}



}  // namespace pidan::test