#include "ksp_plugin/equator_relevance_threshold.hpp"

#include <memory>
#include <string>

#include "astronomy/frames.hpp"
#include "base/not_null.hpp"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "ksp_plugin/frames.hpp"
#include "physics/rotating_body.hpp"
#include "physics/solar_system.hpp"
#include "quantities/astronomy.hpp"
#include "quantities/quantities.hpp"
#include "testing_utilities/approximate_quantity.hpp"
#include "testing_utilities/is_near.hpp"

namespace principia {
namespace ksp_plugin {

using namespace principia::astronomy::_frames;
using namespace principia::base::_not_null;
using namespace principia::ksp_plugin::_equator_relevance_threshold;
using namespace principia::ksp_plugin::_frames;
using namespace principia::physics::_rotating_body;
using namespace principia::physics::_solar_system;
using namespace principia::quantities::_astronomy;
using namespace principia::quantities::_quantities;
using namespace principia::testing_utilities::_approximate_quantity;
using namespace principia::testing_utilities::_is_near;

class EquatorRelevanceThresholdTest : public testing::Test {
 protected:
  EquatorRelevanceThresholdTest()
      : solar_system_j2000_(
            SOLUTION_DIR / "astronomy" / "sol_gravity_model.proto.txt",
            SOLUTION_DIR / "astronomy" /
                "sol_initial_state_jd_2451545_000000000.proto.txt",
            /*ignore_frame=*/true){};

  not_null<std::unique_ptr<RotatingBody<Barycentric>>> MakeBody(
      std::string const& name) {
    return solar_system_j2000_.MakeRotatingBody(
        solar_system_j2000_.gravity_model_message(name));
  }

  Length mean_radius(std::string const& name) {
    return MakeBody(name)->mean_radius();
  }

  SolarSystem<Barycentric> solar_system_j2000_;
};

TEST_F(EquatorRelevanceThresholdTest, Planets) {
  // See the discussion on #1841.
  EXPECT_THAT(EquatorRelevanceThreshold(*MakeBody("Sun")),
              IsNear(58_(1) * SolarRadius));
  EXPECT_THAT(EquatorRelevanceThreshold(*MakeBody("Mercury")),
              IsNear(158_(1) * mean_radius("Mercury")));
  EXPECT_THAT(EquatorRelevanceThreshold(*MakeBody("Venus")),
              IsNear(403_(1) * mean_radius("Venus")));
  EXPECT_THAT(EquatorRelevanceThreshold(*MakeBody("Earth")),
              IsNear(233_(1) * TerrestrialEquatorialRadius));
  EXPECT_THAT(EquatorRelevanceThreshold(*MakeBody("Mars")),
              IsNear(314_(1) * mean_radius("Mars")));
  EXPECT_THAT(EquatorRelevanceThreshold(*MakeBody("Jupiter")),
              IsNear(860_(1) * JovianEquatorialRadius));
  EXPECT_THAT(EquatorRelevanceThreshold(*MakeBody("Saturn")),
              IsNear(938_(1) * mean_radius("Saturn")));
  EXPECT_THAT(EquatorRelevanceThreshold(*MakeBody("Neptune")),
              IsNear(424_(1) * mean_radius("Neptune")));
  EXPECT_THAT(EquatorRelevanceThreshold(*MakeBody("Uranus")),
              IsNear(424_(1) * mean_radius("Uranus")));
}

}  // namespace ksp_plugin
}  // namespace principia
