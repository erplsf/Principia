#pragma once

#include "base/not_null.hpp"
#include "geometry/instant.hpp"
#include "geometry/space.hpp"
#include "physics/degrees_of_freedom.hpp"

namespace principia {
namespace physics {
namespace _trajectory {
namespace internal {

using namespace principia::base::_not_null;
using namespace principia::geometry::_instant;
using namespace principia::geometry::_space;
using namespace principia::physics::_degrees_of_freedom;

template<typename Frame>
class Trajectory {
 public:
  virtual ~Trajectory() = default;

  // The time range for which the trajectory can be evaluated is [t_min, t_max].
  // Note that it is not required that t_min ≤ t_max: for an empty trajectory,
  // t_min = +∞, and t_max = -∞.
  virtual Instant t_min() const = 0;
  virtual Instant t_max() const = 0;

  // Evaluates the trajectory at the given `time`, which must be in
  // [t_min(), t_max()].
  virtual Position<Frame> EvaluatePosition(Instant const& time) const = 0;
  virtual Velocity<Frame> EvaluateVelocity(Instant const& time) const = 0;
  virtual DegreesOfFreedom<Frame> EvaluateDegreesOfFreedom(
      Instant const& time) const = 0;

  // TODO(phl): This should probably declare `value_type` and friends.
};

}  // namespace internal

using internal::Trajectory;

}  // namespace _trajectory
}  // namespace physics
}  // namespace principia
