#include "storage/disk/disk_manager.h"

#include <assert.h>

#include "common/config.h"
#include "common/io.h"
namespace pidan {

DiskManager::DiskManager(const std::string &db_file)
    : db_file_(PosixIOWrapper::Open(db_file, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR)), next_page_id_(0) {
  assert(db_file_ != -1);
}


DiskManager::~DiskManager() { PosixIOWrapper::Close(db_file_); }

void DiskManager::WritePage(page_id_t page_id, const char *data) {
  assert(page_id < next_page_id_);
  PosixIOWrapper::Lseek(db_file_, page_id * PAGE_SIZE, SEEK_SET);
  PosixIOWrapper::WriteFully(db_file_, data, PAGE_SIZE);
}

void DiskManager::ReadPage(page_id_t page_id, char *page_data) {
  assert(page_id < next_page_id_);
  PosixIOWrapper::Lseek(db_file_, page_id * PAGE_SIZE, SEEK_SET);
  PosixIOWrapper::ReadFully(db_file_, page_data, PAGE_SIZE);
}

}  // namespace pidan