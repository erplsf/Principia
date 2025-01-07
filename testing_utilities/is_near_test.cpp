#include "testing_utilities/is_near.hpp"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "quantities/bipm.hpp"
#include "quantities/named_quantities.hpp"
#include "quantities/si.hpp"
#include "testing_utilities/approximate_quantity.hpp"

namespace principia {
namespace testing_utilities {

using namespace principia::quantities::_bipm;

using namespace principia::quantities::_named_quantities;
using namespace principia::quantities::_si;
using namespace principia::testing_utilities::_approximate_quantity;
using namespace principia::testing_utilities::_is_near;

class IsNearTest : public testing::Test {};

TEST_F(IsNearTest, Dimensionless) {
  double const y = e;
  EXPECT_THAT(y, IsNear(2.718_(1)));
  EXPECT_THAT(y, Not(IsNear(3.0_(2))));
}

TEST_F(IsNearTest, Quantity) {
  Speed v = 1 * Knot;
  EXPECT_THAT(v, IsNear(0.514_(1) * Metre / Second));
  EXPECT_THAT(v, Not(IsNear(3.0_(9) * Metre / Second)));
  EXPECT_THAT(v, IsNear(1.0_(1) * NauticalMile / Hour));
  EXPECT_THAT(v, Not(IsNear(1.2_(1) * NauticalMile / Hour)));
}

TEST_F(IsNearTest, Negatives) {
  EXPECT_THAT(π - std::exp(π), IsNear(-20.000_(1)));
}

}  // namespace testing_utilities
}  // namespace principia
