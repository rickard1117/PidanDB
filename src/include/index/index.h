#pragma once

#include "common/vid.h"

namespace pidan {
class Transaction;

class Key {};

class Index {
 public:
  virtual void Insert(const Key &key, VID value, Transaction *txn) = 0;

  virtual VID Find(const Key &key, Transaction *txn) = 0;

  virtual void Scan(const Key &start_key, uint32_t limit, Transaction *txn) = 0;

  virtual void ScanRange(const Key &start_key, const Key &end_key, uint32_t limit, Transaction *txn) = 0;

  virtual void Delete(const Key &key, Transaction *txn) = 0;
};
}  // namespace pidan