#include "buffer/clock_replacer.h"

#include <gtest/gtest.h>

namespace pidan {

TEST(ClockReplacerTest, EmptyReplacer) {
  ClockReplacer replacer;
  frame_id_t id;
  ASSERT_EQ(replacer.Size(), 0);
  ASSERT_FALSE(replacer.Victim(&id));
}

TEST(ClockReplacerTest, UnpinOneFrameThenVictim) {
  ClockReplacer replacer;
  replacer.Unpin(99);
  ASSERT_EQ(replacer.Size(), 1);

  frame_id_t id;
  ASSERT_TRUE(replacer.Victim(&id));
  ASSERT_EQ(id, 99);

  ASSERT_EQ(replacer.Size(), 0);
  ASSERT_FALSE(replacer.Victim(&id));
}

TEST(ClockReplacerTest, PinOneFrame) {
  ClockReplacer replacer;
  replacer.Unpin(99);

  replacer.Pin(99);
  ASSERT_EQ(replacer.Size(), 0);
  frame_id_t id;
  ASSERT_FALSE(replacer.Victim(&id));
}

TEST(ClockReplacerTest, ComplicatedTestCase1) {
  ClockReplacer replacer;
  replacer.Unpin(1);
  replacer.Unpin(2);
  replacer.Unpin(3);
  replacer.Unpin(4);
  replacer.Unpin(5);
  replacer.Unpin(6);
  replacer.Unpin(1);

  ASSERT_EQ(6, replacer.Size());

  frame_id_t value;
  replacer.Victim(&value);
  ASSERT_EQ(1, value);
  replacer.Victim(&value);
  ASSERT_EQ(2, value);
  replacer.Victim(&value);
  ASSERT_EQ(3, value);

  // Pin 3 没什么用，因为3已经被换出。
  replacer.Pin(3);
  replacer.Pin(4);
  ASSERT_EQ(2, replacer.Size());

  replacer.Unpin(4);
  ASSERT_EQ(3, replacer.Size());

  replacer.Victim(&value);
  ASSERT_EQ(2, replacer.Size());
  ASSERT_EQ(5, value);

  replacer.Victim(&value);
  ASSERT_EQ(1, replacer.Size());
  ASSERT_EQ(6, value);

  replacer.Victim(&value);
  ASSERT_EQ(0, replacer.Size());
  ASSERT_EQ(4, value);
}

}  // namespace pidan