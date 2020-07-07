#pragma once

#include <cstddef>
#include <unordered_map>
#include <vector>

#include "buffer/clock_replacer.h"
#include "common/type.h"
#include "storage/page/page.h"

namespace pidan {

class DiskManager;
class ClockReplacer;

// 在磁盘上叫page 在内存中叫frame。page_id用来从磁盘读写page，frame_id用来在内存中定位一个frame
class BufferPoolManager {
 public:
  BufferPoolManager(size_t pool_size, DiskManager *disk_manager, ClockReplacer *replacer);

  Page *FetchPage(page_id_t page_id);

  bool UnpinPage(page_id_t page_id, bool is_dirty);

  // void FlushPage(page_id_t page_id);

  Page *NewPage(page_id_t *page_id);

  // void DeletePage(page_id_t);

  // void FlushAllPages();
 private:
  void Pin(Page &page) { page.pin_count_++; }

  size_t pool_size_;

  Page *frames_;
  std::unordered_map<page_id_t, frame_id_t> page_table_;
  std::vector<frame_id_t> free_list_;  // 所有还没有被使用的frame
  DiskManager *disk_manager_;
  ClockReplacer *replacer_;
};

}  // namespace pidan
