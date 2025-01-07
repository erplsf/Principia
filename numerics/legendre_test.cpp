#include "numerics/legendre.hpp"

#include "gtest/gtest.h"

namespace principia {
namespace numerics {

using namespace principia::numerics::_legendre;

class LegendreTest : public ::testing::Test {};

TEST_F(LegendreTest, P3) {
  auto const p3 = LegendrePolynomial<3>();
  EXPECT_EQ(0, p3(0));
  EXPECT_EQ(1, p3(1));
  EXPECT_EQ(-1, p3(-1));
  EXPECT_EQ(17, p3(2));
}

TEST_F(LegendreTest, P4) {
  auto const p4 = LegendrePolynomial<4>();
  EXPECT_EQ(3.0 / 8.0, p4(0));
  EXPECT_EQ(1, p4(1));
  EXPECT_EQ(1, p4(-1));
  EXPECT_EQ(443.0 / 8.0, p4(2));
  EXPECT_EQ(443.0 / 8.0, p4(-2));
}

TEST_F(LegendreTest, P16) {
  auto const p16 = LegendrePolynomial<16>();
  EXPECT_EQ(6435.0 / 32768.0, p16(0));
}

}  // namespace numerics
}  // namespace principia
