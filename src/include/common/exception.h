#pragma once

#include <stdexcept>

namespace pidan {
class PosixError : public std::runtime_error {
 public:
  explicit PosixError(const std::string &s) : std::runtime_error(s) {}
};
}  // namespace pidan
