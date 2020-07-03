#pragma once

#include <unordered_map>
#include <vector>

#include "common/type.h"

namespace pidan {

// ClockReplacer 实现了page管理的clock替换策略
class ClockReplacer {
 public:
  explicit ClockReplacer();

  // 根据替换策略，移除一个frame。
  // 如果没有可以被移除的frame，返回false。
  bool Victim(frame_id_t *frame_id);

  // 钉住一个frame，使其不会被移除。
  void Pin(frame_id_t frame_id);

  // 释放一个frame。使其可以被写会磁盘。
  void Unpin(frame_id_t frame_id);

  // 还有多少page可以被取出。
  size_t Size() const { return size_; }

 private:
  using frame_flag = std::pair<bool, bool>;

  frame_flag NewFrame() { return frame_flag{false, true}; }

  inline bool IsPin(frame_flag flag) { return flag.first; }

  inline void UnPin(frame_flag &flag) { flag.first = false; }

  inline void Pin(frame_flag &flag) { flag.first = true; }

  inline bool IsRef(frame_flag &flag) { return flag.second; }

  inline void AddRef(frame_flag &flag) { flag.second = true; }

  inline void DecRef(frame_flag &flag) { flag.second = false; }

  std::vector<frame_id_t> clock_;
  std::unordered_map<frame_id_t, frame_flag> pin_table_;  // 记录每个frame是否被pin，以及它们的引用计数
  size_t clock_index_;                                    // 遍历clock_的游标。
  size_t size_;                                           // 可以被换出的page数
};
}  // namespace pidan
