
#include "buffer/buffer_pool_manager.h"

#include "assert.h"
#include "buffer/clock_replacer.h"
#include "storage/disk/disk_manager.h"

namespace pidan {

BufferPoolManager::BufferPoolManager(size_t pool_size, DiskManager *disk_manager, ClockReplacer *replacer)
    : pool_size_(pool_size), disk_manager_(disk_manager), replacer_(new ClockReplacer) {
  frames_ = new Page[pool_size];
  for (size_t i = 0; i < pool_size; i++) {
    free_list_.push_back(static_cast<page_id_t>(i));
  }
}

Page *BufferPoolManager::FetchPage(page_id_t page_id) {
  // 1.     Search the page table for the requested page (P).
  // 1.1    If P exists, pin it and return it immediately.
  // 1.2    If P does not exist, find a replacement page (R) from either the free list or the replacer.
  //        Note that pages are always found from the free list first.
  // 2.     If R is dirty, write it back to the disk.
  // 3.     Delete R from the page table and insert P.
  // 4.     Update P's metadata, read in the page content from disk, and then return a pointer to P.
  assert(page_id != INVALID_PAGE_ID);
  auto it = page_table_.find(page_id);
  if (it != page_table_.end()) {
    Pin(frames_[it->second]);
    return &frames_[it->second];
  }

  // 请求fetch的page_id并不在page_table_里，而是在disk上。
  // 先尝试从free_list中找到空闲的frame，然后将请求的page从磁盘中读取出来。
  if (!free_list_.empty()) {
    auto free_frame_id = free_list_.back();
    free_list_.erase(free_list_.end() - 1);

    Page &free_page = frames_[free_frame_id];
    free_page.ResetPage(page_id);
    disk_manager_->ReadPage(page_id, free_page.GetData());
    page_table_[free_page.page_id_] = free_frame_id;

    Pin(free_page);
    return &free_page;
  }

  // free_list中已经没有空间了，要通过replacer从内存中替换出一个page，将其写入磁盘（如果是dirty的话），
  // 然后从磁盘中读出请求的page
  frame_id_t victim_frame_id;
  if (!replacer_->Victim(&victim_frame_id)) {
    assert(0);
  }
  Page &victim_page = frames_[victim_frame_id];
  if (victim_page.IsDirty()) {
    disk_manager_->WritePage(victim_page.page_id_, victim_page.GetData());
  }
  auto erase_num = page_table_.erase(victim_page.page_id_);
  victim_page.ResetPage(page_id);
  disk_manager_->ReadPage(page_id, victim_page.GetData());
  assert(erase_num == 1);
  page_table_[victim_page.page_id_] = victim_frame_id;
  return &victim_page;
}

bool BufferPoolManager::UnpinPage(page_id_t page_id, bool is_dirty) {
    assert(page_id != INVALID_PAGE_ID);
    return false;
}

Page *BufferPoolManager::NewPage(page_id_t *page_id) {
    
}

}  // namespace pidan
