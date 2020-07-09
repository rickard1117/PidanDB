#pragma once

#include <algorithm>
#include <climits>
#include <functional>
#include <random>
#include <string>
#include <vector>

namespace pidan::test {

using random_bytes_engine = std::independent_bits_engine<std::default_random_engine, CHAR_BIT, unsigned char>;

// https://stackoverflow.com/questions/25298585/efficiently-generating-random-bytes-of-data-in-c11-14
std::string GenRandomString(size_t min_size, size_t max_size) {
  std::random_device rd;   // Will be used to obtain a seed for the random number engine
  std::mt19937 gen(rd());  // Standard mersenne_twister_engine seeded with rd()
  std::uniform_int_distribution<size_t> distrib(min_size, max_size);

  random_bytes_engine rbe;

  auto random = distrib(gen);

  std::string data;
  data.resize(random);
  std::generate(begin(data), end(data), rbe);
  return data;
}

}  // namespace pidan::test