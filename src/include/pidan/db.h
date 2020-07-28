#pragma once

#include <string>

#include "pidan/slice.h"

namespace pidan {

class PidanDB {
 public:
  void Open(const std::string &name, PidanDB **dbptr);

  virtual void Put(const Slice &key, const Slice &value) = 0;

  virtual void Get(const Slice &key, std::string *val) = 0;

  virtual void Delete(const Slice &key) = 0;

  virtual void Begin();

  virtual void Commit();

  virtual void Abort();
};

}  // namespace pidan