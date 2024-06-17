#include "nidavellir.h"

#include "gtest/gtest.h"

TEST(k, ksd) {
    auto* m = new int[3];
    auto l = m[4];
    EXPECT_EQ(l, 3);
}
