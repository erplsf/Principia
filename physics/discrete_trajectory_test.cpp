#include "physics/discrete_trajectory.hpp"

#include <utility>
#include <vector>

#include "astronomy/time_scales.hpp"
#include "base/serialization.hpp"
#include "geometry/frame.hpp"
#include "geometry/instant.hpp"
#include "geometry/space.hpp"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "physics/degrees_of_freedom.hpp"
#include "physics/discrete_trajectory_segment.hpp"
#include "physics/discrete_trajectory_segment_iterator.hpp"
#include "quantities/named_quantities.hpp"
#include "quantities/quantities.hpp"
#include "quantities/si.hpp"
#include "serialization/physics.pb.h"
#include "testing_utilities/almost_equals.hpp"
#include "testing_utilities/approximate_quantity.hpp"
#include "testing_utilities/componentwise.hpp"
#include "testing_utilities/discrete_trajectory_factories.hpp"
#include "testing_utilities/is_near.hpp"
#include "testing_utilities/matchers.hpp"
#include "testing_utilities/numerics_matchers.hpp"
#include "testing_utilities/serialization.hpp"
#include "testing_utilities/string_log_sink.hpp"

namespace principia {
namespace physics {

using ::testing::AllOf;
using ::testing::ElementsAre;
using ::testing::Eq;
using ::testing::HasSubstr;
using ::testing::Not;
using namespace principia::astronomy::_time_scales;
using namespace principia::base::_serialization;
using namespace principia::geometry::_frame;
using namespace principia::geometry::_instant;
using namespace principia::geometry::_space;
using namespace principia::physics::_degrees_of_freedom;
using namespace principia::physics::_discrete_trajectory;
using namespace principia::physics::_discrete_trajectory_segment;
using namespace principia::physics::_discrete_trajectory_segment_iterator;
using namespace principia::quantities::_named_quantities;
using namespace principia::quantities::_quantities;
using namespace principia::quantities::_si;
using namespace principia::testing_utilities::_almost_equals;
using namespace principia::testing_utilities::_approximate_quantity;
using namespace principia::testing_utilities::_componentwise;
using namespace principia::testing_utilities::_discrete_trajectory_factories;
using namespace principia::testing_utilities::_is_near;
using namespace principia::testing_utilities::_matchers;
using namespace principia::testing_utilities::_numerics_matchers;
using namespace principia::testing_utilities::_serialization;
using namespace principia::testing_utilities::_string_log_sink;

class DiscreteTrajectoryTest : public ::testing::Test {
 protected:
  using World = Frame<serialization::Frame::TestTag,
                      Inertial,
                      Handedness::Right,
                      serialization::Frame::TEST>;


  // Constructs a trajectory with three 5-second segments starting at `t0` and
  // the given `degrees_of_freedom`.
  DiscreteTrajectory<World> MakeTrajectory(
      Instant const& t0,
      DegreesOfFreedom<World> const& degrees_of_freedom) {
    DiscreteTrajectory<World> trajectory;
    std::optional<DegreesOfFreedom<World>> last_degrees_of_freedom;

    for (auto const& [t, degrees_of_freedom] :
         NewLinearTrajectoryTimeline(degrees_of_freedom,
                                     /*Δt=*/1 * Second,
                                     /*t1=*/t0,
                                     /*t2=*/t0 + 5 * Second)) {
      last_degrees_of_freedom = degrees_of_freedom;
      EXPECT_OK(trajectory.Append(t, degrees_of_freedom));
    }

    trajectory.NewSegment();
    Velocity<World> const v2({0 * Metre / Second,
                              1 * Metre / Second,
                              0 * Metre / Second});
    for (auto const& [t, degrees_of_freedom] :
        NewLinearTrajectoryTimeline(DegreesOfFreedom<World>(
                                        last_degrees_of_freedom->position(),
                                        v2),
                                     /*Δt=*/1 * Second,
                                     /*t1=*/t0 + 5 * Second,
                                     /*t2=*/t0 + 10 * Second)) {
      last_degrees_of_freedom = degrees_of_freedom;
      EXPECT_OK(trajectory.Append(t, degrees_of_freedom));
    }

    trajectory.NewSegment();
    Velocity<World> const v3({0 * Metre / Second,
                              0 * Metre / Second,
                              1 * Metre / Second});
    for (auto const& [t, degrees_of_freedom] :
        NewLinearTrajectoryTimeline(DegreesOfFreedom<World>(
                                        last_degrees_of_freedom->position(),
                                        v3),
                                     /*Δt=*/1 * Second,
                                     /*t1=*/t0 + 10 * Second,
                                     /*t2=*/t0 + 15 * Second)) {
      EXPECT_OK(trajectory.Append(t, degrees_of_freedom));
    }

    return trajectory;
  }

  DiscreteTrajectory<World> MakeTrajectory() {
    Velocity<World> const v1({1 * Metre / Second,
                              0 * Metre / Second,
                              0 * Metre / Second});
    return MakeTrajectory(t0_, DegreesOfFreedom<World>(World::origin, v1));
  }

  Instant const t0_;
};

TEST_F(DiscreteTrajectoryTest, Make) {
  auto const trajectory = MakeTrajectory();
}

TEST_F(DiscreteTrajectoryTest, BackFront) {
  auto const trajectory = MakeTrajectory();
  EXPECT_EQ(t0_, trajectory.front().time);
  EXPECT_EQ(t0_ + 14 * Second, trajectory.back().time);
}

TEST_F(DiscreteTrajectoryTest, FrontEmpty) {
  // Construct a non-empty trajectory with an empty front segment.
  DiscreteTrajectory<World> trajectory;
  trajectory.NewSegment();
  EXPECT_OK(trajectory.Append(
      t0_, DegreesOfFreedom<World>(World::origin, Velocity<World>())));
  ASSERT_FALSE(trajectory.empty());
  ASSERT_TRUE(trajectory.segments().front().empty());

  // Verify that begin() and front() behave as expected.
  EXPECT_EQ(trajectory.front().time, t0_);
  EXPECT_EQ(trajectory.begin()->time, t0_);

  EXPECT_EQ(trajectory.segments().front().front().time, t0_);
  EXPECT_EQ(trajectory.segments().front().begin()->time, t0_);
}

TEST_F(DiscreteTrajectoryTest, IterateForward) {
  auto const trajectory = MakeTrajectory();
  std::vector<Instant> times;
  for (auto const& [t, _] : trajectory) {
    times.push_back(t);
  }
  EXPECT_THAT(times,
              ElementsAre(t0_,
                          t0_ + 1 * Second,
                          t0_ + 2 * Second,
                          t0_ + 3 * Second,
                          t0_ + 4 * Second,
                          t0_ + 5 * Second,
                          t0_ + 6 * Second,
                          t0_ + 7 * Second,
                          t0_ + 8 * Second,
                          t0_ + 9 * Second,
                          t0_ + 10 * Second,
                          t0_ + 11 * Second,
                          t0_ + 12 * Second,
                          t0_ + 13 * Second,
                          t0_ + 14 * Second));
}

TEST_F(DiscreteTrajectoryTest, IterateBackward) {
  auto const trajectory = MakeTrajectory();
  std::vector<Instant> times;
  for (auto it = trajectory.rbegin(); it != trajectory.rend(); ++it) {
    times.push_back(it->time);
  }
  EXPECT_THAT(times,
              ElementsAre(t0_ + 14 * Second,
                          t0_ + 13 * Second,
                          t0_ + 12 * Second,
                          t0_ + 11 * Second,
                          t0_ + 10 * Second,
                          t0_ + 9 * Second,
                          t0_ + 8 * Second,
                          t0_ + 7 * Second,
                          t0_ + 6 * Second,
                          t0_ + 5 * Second,
                          t0_ + 4 * Second,
                          t0_ + 3 * Second,
                          t0_ + 2 * Second,
                          t0_ + 1 * Second,
                          t0_));
}

TEST_F(DiscreteTrajectoryTest, Empty) {
  DiscreteTrajectory<World> trajectory;
  EXPECT_TRUE(trajectory.empty());
  EXPECT_EQ(trajectory.begin(), trajectory.end());
  trajectory = MakeTrajectory();
  EXPECT_FALSE(trajectory.empty());
  EXPECT_NE(trajectory.begin(), trajectory.end());
}

TEST_F(DiscreteTrajectoryTest, Size) {
  DiscreteTrajectory<World> trajectory;
  EXPECT_EQ(0, trajectory.size());
  trajectory = MakeTrajectory();
  EXPECT_EQ(15, trajectory.size());
}

TEST_F(DiscreteTrajectoryTest, Find) {
  auto const trajectory = MakeTrajectory();
  {
    auto const it = trajectory.find(t0_ + 3 * Second);
    auto const& [t, degrees_of_freedom] = *it;
    EXPECT_EQ(t, t0_ + 3 * Second);
    EXPECT_EQ(degrees_of_freedom.position(),
              World::origin + Displacement<World>({3 * Metre,
                                                   0 * Metre,
                                                   0 * Metre}));
  }
  {
    auto const it = trajectory.find(t0_ + 13 * Second);
    auto const& [t, degrees_of_freedom] = *it;
    EXPECT_EQ(t, t0_ + 13 * Second);
    EXPECT_EQ(degrees_of_freedom.position(),
              World::origin + Displacement<World>({4 * Metre,
                                                   4 * Metre,
                                                   3 * Metre}));
  }
  {
    auto const it = trajectory.find(t0_ + 3.14 * Second);
    EXPECT_TRUE(it == trajectory.end());
  }
}

TEST_F(DiscreteTrajectoryTest, LowerBound) {
  auto const trajectory = MakeTrajectory();
  {
    auto const it = trajectory.lower_bound(t0_ + 3.9 * Second);
    auto const& [t, degrees_of_freedom] = *it;
    EXPECT_EQ(t, t0_ + 4 * Second);
    EXPECT_EQ(degrees_of_freedom.position(),
              World::origin + Displacement<World>({4 * Metre,
                                                   0 * Metre,
                                                   0 * Metre}));
  }
  {
    auto const it = trajectory.lower_bound(t0_ + 4 * Second);
    auto const& [t, degrees_of_freedom] = *it;
    EXPECT_EQ(t, t0_ + 4 * Second);
    EXPECT_EQ(degrees_of_freedom.position(),
              World::origin + Displacement<World>({4 * Metre,
                                                   0 * Metre,
                                                   0 * Metre}));
  }
  {
    auto const it = trajectory.lower_bound(t0_ + 4.1 * Second);
    auto const& [t, degrees_of_freedom] = *it;
    EXPECT_EQ(t, t0_ + 5 * Second);
    EXPECT_EQ(degrees_of_freedom.position(),
              World::origin + Displacement<World>({4 * Metre,
                                                   0 * Metre,
                                                   0 * Metre}));
  }
  {
    auto const it = trajectory.lower_bound(t0_ + 13 * Second);
    auto const& [t, degrees_of_freedom] = *it;
    EXPECT_EQ(t, t0_ + 13 * Second);
    EXPECT_EQ(degrees_of_freedom.position(),
              World::origin + Displacement<World>({4 * Metre,
                                                   4 * Metre,
                                                   3 * Metre}));
  }
  {
    auto const it = trajectory.lower_bound(t0_ + 14.2 * Second);
    EXPECT_TRUE(it == trajectory.end());
  }
  {
    auto const it = trajectory.lower_bound(t0_ - 99 * Second);
    auto const& [t, _] = *it;
    EXPECT_EQ(t, t0_);
  }
}

TEST_F(DiscreteTrajectoryTest, UpperBound) {
  auto const trajectory = MakeTrajectory();
  {
    auto const it = trajectory.upper_bound(t0_ + 3.9 * Second);
    auto const& [t, degrees_of_freedom] = *it;
    EXPECT_EQ(t, t0_ + 4 * Second);
    EXPECT_EQ(degrees_of_freedom.position(),
              World::origin + Displacement<World>({4 * Metre,
                                                   0 * Metre,
                                                   0 * Metre}));
  }
  {
    auto const it = trajectory.upper_bound(t0_ + 4 * Second);
    auto const& [t, degrees_of_freedom] = *it;
    EXPECT_EQ(t, t0_ + 5 * Second);
    EXPECT_EQ(degrees_of_freedom.position(),
              World::origin + Displacement<World>({4 * Metre,
                                                   0 * Metre,
                                                   0 * Metre}));
  }
  {
    auto const it = trajectory.upper_bound(t0_ + 4.1 * Second);
    auto const& [t, degrees_of_freedom] = *it;
    EXPECT_EQ(t, t0_ + 5 * Second);
    EXPECT_EQ(degrees_of_freedom.position(),
              World::origin + Displacement<World>({4 * Metre,
                                                   0 * Metre,
                                                   0 * Metre}));
  }
  {
    auto const it = trajectory.upper_bound(t0_ + 13 * Second);
    auto const& [t, degrees_of_freedom] = *it;
    EXPECT_EQ(t, t0_ + 14 * Second);
    EXPECT_EQ(degrees_of_freedom.position(),
              World::origin + Displacement<World>({4 * Metre,
                                                   4 * Metre,
                                                   4 * Metre}));
  }
  {
    auto const it = trajectory.upper_bound(t0_ + 14.2 * Second);
    EXPECT_TRUE(it == trajectory.end());
  }
  {
    auto const it = trajectory.upper_bound(t0_ - 99 * Second);
    auto const& [t, _] = *it;
    EXPECT_EQ(t, t0_);
  }
}

TEST_F(DiscreteTrajectoryTest, Segments) {
  auto const trajectory = MakeTrajectory();
  std::vector<Instant> begin;
  std::vector<Instant> rbegin;
  for (auto const& sit : trajectory.segments()) {
    begin.push_back(sit.begin()->time);
    rbegin.push_back(sit.rbegin()->time);
  }
  EXPECT_THAT(
      begin,
      ElementsAre(t0_, t0_ + 4 * Second, t0_ + 9 * Second));
  EXPECT_THAT(
      rbegin,
      ElementsAre(t0_ + 4 * Second, t0_ + 9 * Second, t0_ + 14 * Second));
}

TEST_F(DiscreteTrajectoryTest, RSegments) {
  auto const trajectory = MakeTrajectory();
  std::vector<Instant> begin;
  std::vector<Instant> rbegin;
  for (auto const& sit : trajectory.rsegments()) {
    begin.push_back(sit.begin()->time);
    rbegin.push_back(sit.rbegin()->time);
  }
  EXPECT_THAT(
      begin,
      ElementsAre(t0_ + 9 * Second, t0_ + 4 * Second, t0_));
  EXPECT_THAT(
      rbegin,
      ElementsAre(t0_ + 14 * Second, t0_ + 9 * Second, t0_ + 4 * Second));
}

TEST_F(DiscreteTrajectoryTest, DetachSegments) {
  auto trajectory1 = MakeTrajectory();
  auto const first_segment = trajectory1.segments().begin();
  auto const second_segment = std::next(first_segment);
  auto trajectory2 = trajectory1.DetachSegments(second_segment);
  EXPECT_EQ(1, trajectory1.segments().size());
  EXPECT_EQ(2, trajectory2.segments().size());
  EXPECT_EQ(t0_, trajectory1.begin()->time);
  EXPECT_EQ(t0_ + 4 * Second, trajectory1.rbegin()->time);
  EXPECT_EQ(t0_ + 4 * Second, trajectory2.begin()->time);
  EXPECT_EQ(t0_ + 14 * Second, trajectory2.rbegin()->time);

  // Check that the trajectories are minimally usable (in particular, as far as
  // the time-to-segment mapping is concerned).
  {
    auto const it = trajectory1.lower_bound(t0_ + 3.9 * Second);
    auto const& [t, degrees_of_freedom] = *it;
    EXPECT_EQ(t, t0_ + 4 * Second);
    EXPECT_EQ(degrees_of_freedom.position(),
              World::origin + Displacement<World>({4 * Metre,
                                                   0 * Metre,
                                                   0 * Metre}));
  }
  {
    auto const it = trajectory1.lower_bound(t0_ + 4 * Second);
    auto const& [t, degrees_of_freedom] = *it;
    EXPECT_EQ(t, t0_ + 4 * Second);
    EXPECT_EQ(degrees_of_freedom.position(),
              World::origin + Displacement<World>({4 * Metre,
                                                   0 * Metre,
                                                   0 * Metre}));
  }
  {
    auto const it = trajectory2.lower_bound(t0_ + 4 * Second);
    auto const& [t, degrees_of_freedom] = *it;
    EXPECT_EQ(t, t0_ + 4 * Second);
    EXPECT_EQ(degrees_of_freedom.position(),
              World::origin + Displacement<World>({4 * Metre,
                                                   0 * Metre,
                                                   0 * Metre}));
  }
  {
    auto const it = trajectory2.lower_bound(t0_ + 4.1 * Second);
    auto const& [t, degrees_of_freedom] = *it;
    EXPECT_EQ(t, t0_ + 5 * Second);
    EXPECT_EQ(degrees_of_freedom.position(),
              World::origin + Displacement<World>({4 * Metre,
                                                   0 * Metre,
                                                   0 * Metre}));
  }
}

TEST_F(DiscreteTrajectoryTest, AttachSegmentsMatching) {
  auto trajectory1 = MakeTrajectory();
  auto trajectory2 = MakeTrajectory(
      t0_ + 14 * Second,
      DegreesOfFreedom<World>(
          World::origin + Displacement<World>({4 * Metre,
                                               4 * Metre,
                                               4 * Metre}),
          Velocity<World>({0 * Metre / Second,
                           0 * Metre / Second,
                           1 * Metre / Second})));
  trajectory1.AttachSegments(std::move(trajectory2));
  EXPECT_EQ(6, trajectory1.segments().size());
  EXPECT_EQ(t0_, trajectory1.begin()->time);
  EXPECT_EQ(t0_ + 28 * Second, trajectory1.rbegin()->time);

  // Check that the trajectories are minimally usable (in particular, as far as
  // the time-to-segment mapping is concerned).
  {
    auto const it = trajectory1.lower_bound(t0_ + 13.9 * Second);
    auto const& [t, degrees_of_freedom] = *it;
    EXPECT_EQ(t, t0_ + 14 * Second);
    EXPECT_EQ(degrees_of_freedom.position(),
              World::origin + Displacement<World>({4 * Metre,
                                                   4 * Metre,
                                                   4 * Metre}));
  }
  {
    auto const it = trajectory1.lower_bound(t0_ + 14 * Second);
    auto const& [t, degrees_of_freedom] = *it;
    EXPECT_EQ(t, t0_ + 14 * Second);
    EXPECT_EQ(degrees_of_freedom.position(),
              World::origin + Displacement<World>({4 * Metre,
                                                   4 * Metre,
                                                   4 * Metre}));
  }
  {
    auto const it = trajectory1.lower_bound(t0_ + 14.1 * Second);
    auto const& [t, degrees_of_freedom] = *it;
    EXPECT_EQ(t, t0_ + 15 * Second);
    EXPECT_EQ(degrees_of_freedom.position(),
              World::origin + Displacement<World>({4 * Metre,
                                                   4 * Metre,
                                                   5 * Metre}));
  }
}

TEST_F(DiscreteTrajectoryTest, AttachSegmentsMismatching) {
  auto trajectory1 = MakeTrajectory();
  auto trajectory2 = MakeTrajectory(
      t0_ + 15 * Second,
      DegreesOfFreedom<World>(
          World::origin + Displacement<World>({5 * Metre,
                                               5 * Metre,
                                               5 * Metre}),
          Velocity<World>({0 * Metre / Second,
                           0 * Metre / Second,
                           1 * Metre / Second})));
  trajectory1.AttachSegments(std::move(trajectory2));
  EXPECT_EQ(6, trajectory1.segments().size());
  EXPECT_EQ(t0_, trajectory1.begin()->time);
  EXPECT_EQ(t0_ + 29 * Second, trajectory1.rbegin()->time);

  EXPECT_EQ(trajectory1.EvaluatePosition(t0_ + 14 * Second),
            World::origin + Displacement<World>({4 * Metre,
                                                 4 * Metre,
                                                 4 * Metre}));
  EXPECT_EQ(trajectory1.EvaluatePosition(t0_ + 15 * Second),
            World::origin + Displacement<World>({5 * Metre,
                                                 5 * Metre,
                                                 5 * Metre}));
}

TEST_F(DiscreteTrajectoryTest, DeleteSegments) {
  auto trajectory = MakeTrajectory();
  auto const first_segment = trajectory.segments().begin();
  auto second_segment = std::next(first_segment);
  trajectory.DeleteSegments(second_segment);
  EXPECT_EQ(1, trajectory.segments().size());
  EXPECT_EQ(t0_, trajectory.begin()->time);
  EXPECT_EQ(t0_ + 4 * Second, trajectory.rbegin()->time);
  EXPECT_TRUE(second_segment == trajectory.segments().end());
}

TEST_F(DiscreteTrajectoryTest, ForgetAfter) {
  {
    auto trajectory = MakeTrajectory();

    trajectory.ForgetAfter(trajectory.end());
    EXPECT_EQ(3, trajectory.segments().size());

    trajectory.ForgetAfter(t0_ + 12 * Second);
    EXPECT_EQ(3, trajectory.segments().size());
    EXPECT_EQ(t0_, trajectory.begin()->time);
    EXPECT_EQ(t0_ + 11 * Second, trajectory.rbegin()->time);

    trajectory.ForgetAfter(t0_ + 6.1 * Second);
    EXPECT_EQ(2, trajectory.segments().size());
    EXPECT_EQ(t0_, trajectory.begin()->time);
    EXPECT_EQ(t0_ + 6 * Second, trajectory.rbegin()->time);

    trajectory.ForgetAfter(t0_ + 4 * Second);
    EXPECT_EQ(1, trajectory.segments().size());
    EXPECT_EQ(t0_, trajectory.begin()->time);
    EXPECT_EQ(t0_ + 4 * Second, trajectory.rbegin()->time);

    trajectory.ForgetAfter(t0_);
    EXPECT_TRUE(trajectory.empty());
    EXPECT_EQ(1, trajectory.segments().size());
  }
  {
    // This used to fail because ForgetAfter would leave a 1-point segment at
    // t0_ + 9 * Second which was not in the time-to-segment map.
    auto trajectory = MakeTrajectory();

    trajectory.ForgetBefore(t0_ + 9 * Second);
    trajectory.ForgetAfter(t0_ + 9 * Second);
  }
}

TEST_F(DiscreteTrajectoryTest, ForgetBefore) {
  auto trajectory = MakeTrajectory();

  trajectory.ForgetBefore(t0_ + 3 * Second);
  EXPECT_EQ(3, trajectory.segments().size());
  EXPECT_EQ(t0_ + 3 * Second, trajectory.begin()->time);
  EXPECT_EQ(t0_ + 14 * Second, trajectory.rbegin()->time);
  EXPECT_EQ(t0_ + 3 * Second, trajectory.t_min());
  EXPECT_EQ(12, trajectory.size());

  trajectory.ForgetBefore(t0_ + 6.1 * Second);
  EXPECT_EQ(3, trajectory.segments().size());
  EXPECT_EQ(t0_ + 7 * Second, trajectory.begin()->time);
  EXPECT_EQ(t0_ + 14 * Second, trajectory.rbegin()->time);
  EXPECT_EQ(t0_ + 7 * Second, trajectory.t_min());
  EXPECT_EQ(8, trajectory.size());

  trajectory.ForgetBefore(t0_ + 9 * Second);
  EXPECT_EQ(3, trajectory.segments().size());
  EXPECT_EQ(t0_ + 9 * Second, trajectory.begin()->time);
  EXPECT_EQ(t0_ + 14 * Second, trajectory.rbegin()->time);
  EXPECT_EQ(t0_ + 9 * Second, trajectory.t_min());
  EXPECT_EQ(6, trajectory.size());

  // The trajectory now has empty segments, so let's check that we can properly
  // iterate over those.
  {
    std::vector<Instant> times;
    for (auto const& [t, _] : trajectory) {
      times.push_back(t);
    }
    EXPECT_THAT(times,
                ElementsAre(t0_ + 9 * Second,
                            t0_ + 10 * Second,
                            t0_ + 11 * Second,
                            t0_ + 12 * Second,
                            t0_ + 13 * Second,
                            t0_ + 14 * Second));
  }
  {
    std::vector<Instant> times;
    for (auto it = trajectory.rbegin(); it != trajectory.rend(); ++it) {
      times.push_back(it->time);
    }
    EXPECT_THAT(times,
                ElementsAre(t0_ + 14 * Second,
                            t0_ + 13 * Second,
                            t0_ + 12 * Second,
                            t0_ + 11 * Second,
                            t0_ + 10 * Second,
                            t0_ + 9 * Second));
  }

  trajectory.ForgetBefore(t0_ + 99 * Second);
  EXPECT_TRUE(trajectory.empty());
  EXPECT_EQ(InfiniteFuture, trajectory.t_min());
  EXPECT_EQ(0, trajectory.size());

  trajectory.ForgetBefore(trajectory.end());
  EXPECT_TRUE(trajectory.empty());
  EXPECT_EQ(InfiniteFuture, trajectory.t_min());
  EXPECT_EQ(0, trajectory.size());
}

TEST_F(DiscreteTrajectoryTest, Merge) {
  {
    auto trajectory1 = MakeTrajectory();
    auto trajectory2 = MakeTrajectory();

    trajectory1.ForgetAfter(t0_ + 6 * Second);
    trajectory2.ForgetBefore(t0_ + 6 * Second);

    trajectory1.Merge(std::move(trajectory2));

    EXPECT_EQ(3, trajectory1.segments().size());
    auto sit = trajectory1.segments().begin();
    EXPECT_EQ(5, sit->size());
    EXPECT_EQ(t0_, sit->front().time);
    EXPECT_EQ(t0_ + 4 * Second, sit->back().time);
    ++sit;
    EXPECT_EQ(6, sit->size());
    EXPECT_EQ(t0_ + 4 * Second, sit->front().time);
    EXPECT_EQ(t0_ + 9 * Second, sit->back().time);
    ++sit;
    EXPECT_EQ(6, sit->size());
    EXPECT_EQ(t0_ + 9 * Second, sit->front().time);
    EXPECT_EQ(t0_ + 14 * Second, sit->back().time);
  }
  {
    auto trajectory1 = MakeTrajectory();
    auto trajectory2 = MakeTrajectory();

    trajectory1.ForgetAfter(t0_ + 6 * Second);
    trajectory2.ForgetBefore(t0_ + 6 * Second);

    trajectory2.Merge(std::move(trajectory1));

    EXPECT_EQ(3, trajectory2.segments().size());
    auto sit = trajectory2.segments().begin();
    EXPECT_EQ(5, sit->size());
    EXPECT_EQ(t0_, sit->front().time);
    EXPECT_EQ(t0_ + 4 * Second, sit->back().time);
    ++sit;
    EXPECT_EQ(6, sit->size());
    EXPECT_EQ(t0_ + 4 * Second, sit->front().time);
    EXPECT_EQ(t0_ + 9 * Second, sit->back().time);
    ++sit;
    EXPECT_EQ(6, sit->size());
    EXPECT_EQ(t0_ + 9 * Second, sit->front().time);
    EXPECT_EQ(t0_ + 14 * Second, sit->back().time);
  }
  {
    auto trajectory1 = MakeTrajectory();
    auto trajectory2 = MakeTrajectory();

    trajectory1.ForgetAfter(t0_ + 9 * Second);
    // This trajectory starts with a 1-point segment.  Merge used to fail the
    // consistency check because the time-to-segment map was losing an entry.
    trajectory2.ForgetBefore(t0_ + 9 * Second);

    trajectory2.Merge(std::move(trajectory1));
  }
  {
    // This used to fail a consistency check because the segments of the target
    // that follow the end of the source were not processed, and the time-to-
    // segment map was left inconsistent.
    auto trajectory1 = MakeTrajectory();
    auto trajectory2 = MakeTrajectory();

    trajectory1.ForgetBefore(t0_ + 4 * Second);
    auto sit = std::next(trajectory1.segments().begin());
    trajectory1.DeleteSegments(sit);
    trajectory2.ForgetBefore(t0_ + 4 * Second);

    trajectory2.Merge(std::move(trajectory1));
  }
}

TEST_F(DiscreteTrajectoryTest, TMinTMaxEvaluate) {
  auto const trajectory = MakeTrajectory();
  EXPECT_EQ(t0_, trajectory.t_min());
  EXPECT_EQ(t0_ + 14 * Second, trajectory.t_max());
  EXPECT_THAT(trajectory.EvaluateDegreesOfFreedom(t0_ + 3.14 * Second),
      Componentwise(AlmostEquals(
                        World::origin + Displacement<World>({3.14 * Metre,
                                                             0 * Metre,
                                                             0 * Metre}), 0),
                    AlmostEquals(Velocity<World>({1 * Metre / Second,
                                                  0 * Metre / Second,
                                                  0 * Metre / Second}), 0)));
  EXPECT_THAT(trajectory.EvaluateDegreesOfFreedom(t0_ + 6.78 * Second),
      Componentwise(AlmostEquals(
                        World::origin + Displacement<World>({4 * Metre,
                                                             1.78 * Metre,
                                                             0 * Metre}), 1),
                    AlmostEquals(Velocity<World>({0 * Metre / Second,
                                        1 * Metre / Second,
                                        0 * Metre / Second}), 0)));
}

TEST_F(DiscreteTrajectoryTest, SerializationRoundTrip) {
  auto const trajectory = MakeTrajectory();
  auto const trajectory_first_segment = trajectory.segments().begin();
  auto const trajectory_second_segment = std::next(trajectory_first_segment);
  auto const trajectory_past_the_end = trajectory.segments().end();

  serialization::DiscreteTrajectory message1;
  trajectory.WriteToMessage(&message1,
                            /*tracked=*/{trajectory_second_segment,
                                         trajectory_past_the_end},
                            /*exact=*/
                            {trajectory.lower_bound(t0_ + 2 * Second),
                             trajectory.lower_bound(t0_ + 3 * Second)});

  DiscreteTrajectorySegmentIterator<World> deserialized_second_segment;
  DiscreteTrajectorySegmentIterator<World> deserialized_past_the_end;
  auto const deserialized_trajectory =
      DiscreteTrajectory<World>::ReadFromMessage(
          message1, /*tracked=*/{&deserialized_second_segment,
                                 &deserialized_past_the_end});

  // Check that the tracked segment was properly retrieved.
  EXPECT_EQ(t0_ + 4 * Second, deserialized_second_segment->begin()->time);
  EXPECT_EQ(t0_ + 9 * Second, deserialized_second_segment->rbegin()->time);

  // Check that the past-the-end iterator was properly set.
  EXPECT_TRUE(deserialized_past_the_end ==
              deserialized_trajectory.segments().end());

  // Check that the exact points are exact.
  EXPECT_EQ(
      deserialized_trajectory.lower_bound(t0_ + 2 * Second)->degrees_of_freedom,
      trajectory.lower_bound(t0_ + 2 * Second)->degrees_of_freedom);
  EXPECT_EQ(
      deserialized_trajectory.lower_bound(t0_ + 3 * Second)->degrees_of_freedom,
      trajectory.lower_bound(t0_ + 3 * Second)->degrees_of_freedom);

  serialization::DiscreteTrajectory message2;
  deserialized_trajectory.WriteToMessage(
      &message2,
      /*tracked=*/{deserialized_second_segment,
                   deserialized_past_the_end},
      /*exact=*/
      {deserialized_trajectory.lower_bound(t0_ + 2 * Second),
       deserialized_trajectory.lower_bound(t0_ + 3 * Second)});

  EXPECT_THAT(message2, EqualsProto(message1));
}

TEST_F(DiscreteTrajectoryTest, SerializationExactEndpoints) {
  DiscreteTrajectory<World> trajectory;
  AngularFrequency const ω = 3 * Radian / Second;
  Length const r = 2 * Metre;
  Time const Δt = 1.0 / 3.0 * Milli(Second);
  Instant const t1 = t0_;
  Instant const t2 = t0_ + 100.0 / 7.0 * Second;
  Instant const t3 = t0_ + 200.0 / 11.0 * Second;
  // Downsampling is required for ZFP compression.
  DiscreteTrajectorySegment<World>::DownsamplingParameters const
      downsampling_parameters{.max_dense_intervals = 100,
                              .tolerance = 5 * Milli(Metre)};

  auto sit = trajectory.segments().begin();
  sit->SetDownsampling(downsampling_parameters);
  AppendTrajectoryTimeline(
      NewCircularTrajectoryTimeline<World>(ω, r, Δt, t1, t2),
      trajectory);
  sit = trajectory.NewSegment();
  sit->SetDownsampling(downsampling_parameters);
  AppendTrajectoryTimeline(
      NewCircularTrajectoryTimeline<World>(2 * ω, 2 * r, Δt, t2, t3),
      trajectory);

  auto const degrees_of_freedom1 =
      trajectory.EvaluateDegreesOfFreedom(t1 + 10 * Second);
  auto const degrees_of_freedom2 =
      trajectory.EvaluateDegreesOfFreedom(t2 + 2 * Second);

  serialization::DiscreteTrajectory message;
  trajectory.WriteToMessage(&message, /*tracked=*/{}, /*exact=*/{});

  // Deserialization would fail if the endpoints were nudged by ZFP compression.
  auto const deserialized_trajectory =
      DiscreteTrajectory<World>::ReadFromMessage(message, /*tracked=*/{});

  auto const deserialized_degrees_of_freedom1 =
      deserialized_trajectory.EvaluateDegreesOfFreedom(t1 + 10 * Second);
  auto const deserialized_degrees_of_freedom2 =
      deserialized_trajectory.EvaluateDegreesOfFreedom(t2 + 2 * Second);

  // These checks verify that ZFP compression actually happened (so we observe
  // small errors on the degrees of freedom).
  EXPECT_THAT(
      (deserialized_degrees_of_freedom1.position() - World::origin).Norm(),
      AbsoluteErrorFrom((degrees_of_freedom1.position() - World::origin).Norm(),
                        IsNear(0.022_(1)*Milli(Metre))));
  EXPECT_THAT(deserialized_degrees_of_freedom1.velocity().Norm(),
              AbsoluteErrorFrom(degrees_of_freedom1.velocity().Norm(),
                                IsNear(5.8_(1) * Milli(Metre) / Second)));
  EXPECT_THAT(
      (deserialized_degrees_of_freedom2.position() - World::origin).Norm(),
      AbsoluteErrorFrom((degrees_of_freedom2.position() - World::origin).Norm(),
                        IsNear(0.47_(1)*Milli(Metre))));
  EXPECT_THAT(deserialized_degrees_of_freedom2.velocity().Norm(),
              AbsoluteErrorFrom(degrees_of_freedom2.velocity().Norm(),
                                IsNear(1.5_(1) * Milli(Metre) / Second)));
}

TEST_F(DiscreteTrajectoryTest, SerializationRange) {
  auto const trajectory1 = MakeTrajectory();
  auto trajectory2 = MakeTrajectory();

  serialization::DiscreteTrajectory message1;
  trajectory1.WriteToMessage(
      &message1,
      /*begin=*/trajectory1.upper_bound(t0_ + 6 * Second),
      /*end=*/trajectory1.upper_bound(t0_ + 12 * Second),
      /*tracked=*/{},
      /*exact=*/{});

  serialization::DiscreteTrajectory message2;
  trajectory2.ForgetBefore(trajectory2.upper_bound(t0_ + 6 * Second));
  trajectory2.ForgetAfter(trajectory2.upper_bound(t0_ + 12 * Second));
  trajectory2.WriteToMessage(&message2,
                             /*tracked=*/{},
                             /*exact=*/{});

  // Writing a range of the trajectory is equivalent to forgetting and writing
  // the result.
  EXPECT_THAT(message1, EqualsProto(message2));
}

TEST_F(DiscreteTrajectoryTest, DISABLED_SerializationPreHamiltonCompatibility) {
  StringLogSink log_warning(google::WARNING);
  auto const serialized_message = ReadFromBinaryFile(
      R"(P:\Public Mockingbird\Principia\Saves\3136\trajectory_3136.proto.bin)");  // NOLINT
  auto const message1 =
      ParseFromBytes<serialization::DiscreteTrajectory>(serialized_message);
  DiscreteTrajectory<World>::SegmentIterator psychohistory;
  auto const history = DiscreteTrajectory<World>::ReadFromMessage(
      message1, /*tracked=*/{&psychohistory});
  EXPECT_THAT(log_warning.string(),  // NOLINT
              AllOf(HasSubstr("pre-Hamilton"), Not(HasSubstr("pre-Haar"))));

  // Note that the sizes don't have the same semantics as pre-Hamilton.  The
  // history now counts all segments.  The psychohistory has a duplicated point
  // at the beginning.
  EXPECT_EQ(435'929, history.size());
  EXPECT_EQ(3, psychohistory->size());

  // Evaluate a point in each of the two segments.
  EXPECT_THAT(
      history.EvaluateDegreesOfFreedom("1957-10-04T19:28:34"_TT),
      Eq(DegreesOfFreedom<World>(
          World::origin +
              Displacement<World>({+1.47513683827317657e+11 * Metre,
                                   +2.88696086355042419e+10 * Metre,
                                   +1.24740082262952404e+10 * Metre}),
          Velocity<World>({-6.28845231836519179e+03 * (Metre / Second),
                           +2.34046542233168329e+04 * (Metre / Second),
                           +4.64410011408655919e+03 * (Metre / Second)}))));
  EXPECT_THAT(
      psychohistory->EvaluateDegreesOfFreedom("1958-10-07T09:38:30"_TT),
      Eq(DegreesOfFreedom<World>(
          World::origin +
              Displacement<World>({+1.45814173315801941e+11 * Metre,
                                   +3.45409490426372147e+10 * Metre,
                                   +1.49445864962450924e+10 * Metre}),
          Velocity<World>({-8.70708379504568074e+03 * (Metre / Second),
                           +2.61488327506437054e+04 * (Metre / Second),
                           +1.90319283138508908e+04 * (Metre / Second)}))));

  // Serialize the trajectory in the Hamilton format.
  serialization::DiscreteTrajectory message2;
  history.WriteToMessage(&message2,
                         /*tracked=*/{psychohistory},
                         /*exact=*/{});
}

}  // namespace physics
}  // namespace principia
