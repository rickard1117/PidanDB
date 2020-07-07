#pragma once

#include <cstring>

#include "common/config.h"
#include "common/type.h"

namespace pidan {

// Page是内存中所有类型page的基类。可以存放4KB的内容，由各自类型自己指定。
class Page {
 public:
  friend class BufferPoolManager;
  Page() { ResetMemory(); }
  virtual ~Page() = default;

  char *GetData() { return data_; }

  bool IsDirty() { return is_dirty_; }

  void ResetMemory() { std::memset(data_, 0, PAGE_SIZE); }

  void ResetPage(page_id_t page_id) {
    ResetMemory();
    page_id_ = page_id;
    pin_count_ = 0;
    is_dirty_ = false;
  }

 private:
  char data_[PAGE_SIZE];
  page_id_t page_id_ = INVALID_PAGE_ID;
  int pin_count_ = 0;
  bool is_dirty_ = false;
};
}  // namespace pidan