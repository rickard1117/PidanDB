#pragma once
#include <string>

#include "common/type.h"

namespace pidan {
class DiskManager {
 public:
  DiskManager(const std::string &db_file);

  void WritePage(page_id_t page_id, const char *data);

  void ReadPage(page_id_t page_id, char *page_data);

  page_id_t AllocatePage() { return next_page_id_++; }

 private:
  int db_file_;
  page_id_t next_page_id_;
};
}  // namespace pidan
