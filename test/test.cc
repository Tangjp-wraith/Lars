#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "lib.h"

TEST(Test1,test) {
  auto a = A(1);
  auto b = A(2);
  EXPECT_EQ(1,1);
}