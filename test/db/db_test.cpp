#include "pidan/db.h"

#include <gtest/gtest.h>

namespace pidan {

TEST(DBTest, SimplePutAndGet) {
  PidanDB *db = nullptr;
  ASSERT_EQ(Status::SUCCESS, PidanDB::Open("test.db", &db));
  ASSERT_EQ(Status::SUCCESS, db->Put("abc", "123"));
  std::string temp_val;
  ASSERT_EQ(Status::SUCCESS, db->Get("abc", &temp_val));
  ASSERT_EQ(temp_val, "123");
  ASSERT_EQ(Status::SUCCESS, db->Put("abc", "234"));
  ASSERT_EQ(Status::SUCCESS, db->Get("abc", &temp_val));
  ASSERT_EQ(temp_val, "234");
  ASSERT_EQ(Status::KEY_NOT_EXIST, db->Get("not_key", &temp_val));
}

}  // namespace pidan
