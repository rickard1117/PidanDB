#include "storage/disk/disk_manager.h"

#include <gtest/gtest.h>
#include <stdio.h>

#include "common/config.h"
namespace pidan {

static void DeleteFile(const std::string &filename) { remove(filename.c_str()); }

class DiskManagerTest : public ::testing::Test {
 protected:
  void SetUp() override {
    DeleteFile("test.db");
    dm_ = new DiskManager("test.db");
  }

  void TearDown() override {
    delete dm_;
    DeleteFile("test.db");
  }

  DiskManager *dm_;
};

TEST_F(DiskManagerTest, WriteReadPage) {
  auto page_id_1 = dm_->AllocatePage();
  ASSERT_EQ(page_id_1, 0);
  std::string data(PAGE_SIZE, 'd');
  std::string data2;
  data2.resize(PAGE_SIZE);
  dm_->WritePage(page_id_1, data.data());
  dm_->ReadPage(page_id_1, data2.data());
  ASSERT_EQ(data.compare(data2), 0);
}

}  // namespace pidan