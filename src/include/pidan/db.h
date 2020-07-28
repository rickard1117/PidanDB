#pragma once

#include <string>

#include "pidan/slice.h"
#include "pidan/errors.h"

namespace pidan {

class PidanDB {
 public:
  PidanDB() = default;

  virtual ~PidanDB();

  static Status Open(const std::string &name, PidanDB **dbptr);

  virtual Status Put(const Slice &key, const Slice &value) = 0;

  virtual Status Get(const Slice &key, std::string *val) = 0;

  virtual Status Delete(const Slice &key) = 0;

  virtual Status Begin() = 0;

  virtual Status Commit() = 0;

  virtual Status Abort() = 0;
};

}  // namespace pidan