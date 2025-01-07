#include "physics/discrete_trajectory_segment_iterator.hpp"

#include <iterator>
#include <memory>

#include "base/not_null.hpp"
#include "geometry/frame.hpp"
#include "geometry/instant.hpp"
#include "gtest/gtest.h"
#include "physics/degrees_of_freedom.hpp"
#include "physics/discrete_trajectory_segment.hpp"
#include "quantities/si.hpp"
#include "testing_utilities/matchers.hpp"  // 🧙 For EXPECT_OK.

namespace principia {
namespace physics {

using ::testing::Return;
using namespace principia::base::_not_null;
using namespace principia::geometry::_frame;
using namespace principia::geometry::_instant;
using namespace principia::physics::_degrees_of_freedom;
using namespace principia::physics::_discrete_trajectory_segment;
using namespace principia::physics::_discrete_trajectory_segment_iterator;
using namespace principia::quantities::_si;

class DiscreteTrajectorySegmentIteratorTest : public ::testing::Test {
 protected:
  using World = Frame<struct WorldTag>;
  using Segments = _discrete_trajectory_types::Segments<World>;
  static_assert(
      std::bidirectional_iterator<DiscreteTrajectorySegmentIterator<World>>);

  DiscreteTrajectorySegmentIteratorTest()
      : segments_(MakeSegments(3)) {
    auto it = segments_->begin();
    {
      auto& segment1 = *it;
      EXPECT_OK(segment1.Append(t0_ + 2 * Second, unmoving_origin_));
      EXPECT_OK(segment1.Append(t0_ + 3 * Second, unmoving_origin_));
      EXPECT_OK(segment1.Append(t0_ + 5 * Second, unmoving_origin_));
      EXPECT_OK(segment1.Append(t0_ + 7 * Second, unmoving_origin_));
      EXPECT_OK(segment1.Append(t0_ + 11 * Second, unmoving_origin_));
    }

    ++it;
    {
      auto& segment2 = *it;
      EXPECT_OK(segment2.Append(t0_ + 13 * Second, unmoving_origin_));
    }

    ++it;
    {
      auto& segment3 = *it;
      EXPECT_OK(segment3.Append(t0_ + 13 * Second, unmoving_origin_));
      EXPECT_OK(segment3.Append(t0_ + 17 * Second, unmoving_origin_));
      EXPECT_OK(segment3.Append(t0_ + 19 * Second, unmoving_origin_));
    }
  }

  DiscreteTrajectorySegmentIterator<World> MakeIterator(
      not_null<Segments*> const segments,
      Segments::iterator const iterator) {
    return DiscreteTrajectorySegmentIterator<World>(segments, iterator);
  }

  // Constructs a list of `n` segments which are properly initialized.
  // TODO(phl): Move to a central place.
  static not_null<std::unique_ptr<Segments>> MakeSegments(const int n) {
    auto segments = make_not_null_unique<Segments>(n);
    for (auto it = segments->begin(); it != segments->end(); ++it) {
      *it = DiscreteTrajectorySegment<World>(
          DiscreteTrajectorySegmentIterator<World>(segments.get(), it));
    }
    return segments;
  }

  not_null<std::unique_ptr<Segments>> segments_;
  Instant const t0_;
  DegreesOfFreedom<World> const unmoving_origin_{World::origin,
                                                 World::unmoving};
};

TEST_F(DiscreteTrajectorySegmentIteratorTest, Basic) {
  {
    auto iterator = MakeIterator(segments_.get(), segments_->begin());
    EXPECT_EQ(5, iterator->size());
    auto const current = ++iterator;
    EXPECT_EQ(1, iterator->size());
    EXPECT_EQ(1, current->size());
    auto const previous = iterator++;
    EXPECT_EQ(3, iterator->size());
    EXPECT_EQ(1, previous->size());
  }
  {
    auto iterator = MakeIterator(segments_.get(), segments_->end());
    --iterator;
    EXPECT_EQ(3, (*iterator).size());
    auto const current = --iterator;
    EXPECT_EQ(1, (*iterator).size());
    EXPECT_EQ(1, (*current).size());
    auto const previous = iterator--;
    EXPECT_EQ(5, (*iterator).size());
    EXPECT_EQ(1, (*previous).size());
  }
}

}  // namespace physics
}  // namespace principia
