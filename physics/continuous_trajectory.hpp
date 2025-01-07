#pragma once

#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include "absl/status/status.h"
#include "absl/synchronization/mutex.h"
#include "base/concepts.hpp"
#include "base/not_null.hpp"
#include "geometry/instant.hpp"
#include "geometry/space.hpp"
#include "numerics/piecewise_poisson_series.hpp"
#include "numerics/polynomial.hpp"
#include "numerics/polynomial_evaluators.hpp"
#include "numerics/polynomial_in_monomial_basis.hpp"
#include "physics/checkpointer.hpp"
#include "physics/degrees_of_freedom.hpp"
#include "physics/trajectory.hpp"
#include "quantities/quantities.hpp"
#include "serialization/physics.pb.h"

namespace principia {
namespace physics {

template<typename Frame>
class TestableContinuousTrajectory;

namespace _continuous_trajectory {
namespace internal {

using namespace principia::base::_concepts;
using namespace principia::base::_not_null;
using namespace principia::geometry::_instant;
using namespace principia::geometry::_space;
using namespace principia::numerics::_piecewise_poisson_series;
using namespace principia::numerics::_polynomial;
using namespace principia::numerics::_polynomial_evaluators;
using namespace principia::numerics::_polynomial_in_monomial_basis;
using namespace principia::physics::_checkpointer;
using namespace principia::physics::_degrees_of_freedom;
using namespace principia::physics::_trajectory;
using namespace principia::quantities::_quantities;

// This class is thread-safe, but the client must be aware that if, for
// instance, the trajectory is appended to asynchronously, successive calls to
// `t_max()` may return different values.
template<typename Frame>
class ContinuousTrajectory : public Trajectory<Frame> {
 public:
  // Constructs a trajectory with the given time `step`.  Because the Чебышёв
  // polynomials have values in the range [-1, 1], the error resulting of
  // truncating the infinite Чебышёв series to a finite degree are a small
  // multiple of the coefficient of highest degree (assuming that the series
  // converges reasonably well).  Thus, we pick the degree of the series so that
  // the coefficient of highest degree is less than `tolerance`.
  ContinuousTrajectory(Time const& step,
                       Length const& tolerance);

  ContinuousTrajectory(ContinuousTrajectory const&) = delete;
  ContinuousTrajectory(ContinuousTrajectory&&) = delete;
  ContinuousTrajectory& operator=(ContinuousTrajectory const&) = delete;
  ContinuousTrajectory& operator=(ContinuousTrajectory&&) = delete;

  // Returns true iff this trajectory cannot be evaluated for any time.
  bool empty() const EXCLUDES(lock_);

  // The average degree of the polynomials for the trajectory.  Only useful for
  // benchmarking or analyzing performance.  Do not use in real code.
  double average_degree() const EXCLUDES(lock_);

  // Appends one point to the trajectory.  `time` must be after the last time
  // passed to `Append` if the trajectory is not empty.  The `time`s passed to
  // successive calls to `Append` must be equally spaced with the `step` given
  // at construction.
  absl::Status Append(Instant const& time,
                      DegreesOfFreedom<Frame> const& degrees_of_freedom)
      EXCLUDES(lock_);

  // Prepends the given `trajectory` to this one.  Ideally the last point of
  // `trajectory` should match the first point of this object.
  // Note the rvalue reference: `ContinuousTrajectory` is not moveable and not
  // copyable, but the `InstantPolynomialPairs` are moveable and we really want
  // to move them.  We could pass by non-const lvalue reference, but we would
  // rather make it clear at the calling site that the object is consumed, so
  // we require the use of std::move.
  void Prepend(ContinuousTrajectory&& trajectory);

  // Implementation of the interface `Trajectory`.

  // `t_max` may be less than the last time passed to Append because the
  // trajectory cannot be evaluated for the last points, for which no polynomial
  // was constructed.  For an empty trajectory, an infinity with the proper
  // sign is returned.
  Instant t_min() const override EXCLUDES(lock_);
  Instant t_max() const override EXCLUDES(lock_);

  Position<Frame> EvaluatePosition(Instant const& time) const override
      EXCLUDES(lock_);
  Velocity<Frame> EvaluateVelocity(Instant const& time) const override
      EXCLUDES(lock_);
  DegreesOfFreedom<Frame> EvaluateDegreesOfFreedom(
      Instant const& time) const override EXCLUDES(lock_);

  // End of the implementation of the interface.

#if PRINCIPIA_CONTINUOUS_TRAJECTORY_SUPPORTS_PIECEWISE_POISSON_SERIES
  // Returns the degree for a piecewise Poisson series covering the given time
  // interval.
  int PiecewisePoissonSeriesDegree(Instant const& t_min,
                                   Instant const& t_max) const;

  // Computes a piecewise Poisson series covering the given time interval.  The
  // degree must be at least the one returned by the preceding function.
  template<int aperiodic_degree, int periodic_degree>
  PiecewisePoissonSeries<Displacement<Frame>,
                         aperiodic_degree, periodic_degree,
                         EstrinEvaluator>
  ToPiecewisePoissonSeries(Instant const& t_min,
                           Instant const& t_max) const;
#endif

  void WriteToMessage(not_null<serialization::ContinuousTrajectory*> message)
      const EXCLUDES(lock_);
  // The parameter `desired_t_min` indicates that the trajectory must be
  // restored at a checkpoint such that, once it is appended to, its t_min() is
  // at or before `desired_t_min`.
  static not_null<std::unique_ptr<ContinuousTrajectory>> ReadFromMessage(
      Instant const& desired_t_min,
      serialization::ContinuousTrajectory const& message)
    requires serializable<Frame>;

  // These members call the corresponding functions of the internal
  // checkpointer.
  void WriteToCheckpoint(Instant const& t) const;
  absl::Status ReadFromCheckpointAt(
      Instant const& t,
      Checkpointer<serialization::ContinuousTrajectory>::Reader const& reader)
      const;

  // Return functions that can be passed to a `Checkpointer` to write this
  // trajectory to a checkpoint or read it back.
  Checkpointer<serialization::ContinuousTrajectory>::Writer
  MakeCheckpointerWriter();
  Checkpointer<serialization::ContinuousTrajectory>::Reader
  MakeCheckpointerReader();

 public:
  // Beware! This part of the API is thread-unsafe.  The caller (which may be a
  // function of this class) is responsible for synchronization, i.e., for
  // making sure that no mutators execute in parallel with any of the functions
  // having "locked" in their name.  The purpose of this API is to improve the
  // performance of the `Ephemeris`.

  Instant t_min_locked() const;
  Instant t_max_locked() const;

  Position<Frame> EvaluatePositionLocked(Instant const& time) const;
  Velocity<Frame> EvaluateVelocityLocked(Instant const& time) const;
  DegreesOfFreedom<Frame> EvaluateDegreesOfFreedomLocked(
      Instant const& time) const;

 protected:
  // For mocking.
  ContinuousTrajectory();

 private:
  // Each polynomial is valid over an interval [t_min, t_max].  Polynomials are
  // stored in this vector sorted by their `t_max`, as it turns out that we
  // never need to extract their `t_min`.  Logically, the `t_min` for a
  // polynomial is the `t_max` of the previous one.  The first polynomial has a
  // `t_min` which is `*first_time_`.
  struct InstantPolynomialPair {
    InstantPolynomialPair(
        Instant t_max,
        not_null<std::unique_ptr<Polynomial<Position<Frame>, Instant>>>
            polynomial);
    Instant t_max;
    not_null<std::unique_ptr<Polynomial<Position<Frame>, Instant>>>
        polynomial;
  };
  using InstantPolynomialPairs = std::vector<InstantPolynomialPair>;

  // Really a static method, but may be overridden for testing.
  virtual not_null<std::unique_ptr<Polynomial<Position<Frame>, Instant>>>
  NewhallApproximationInMonomialBasis(
      int degree,
      std::vector<Position<Frame>> const& q,
      std::vector<Velocity<Frame>> const& v,
      Instant const& t_min,
      Instant const& t_max,
      Displacement<Frame>& error_estimate) const;

  // Computes the best Newhall approximation based on the desired tolerance.
  // Adjust the `degree_` and other member variables to stay within the
  // tolerance while minimizing the computational cost and avoiding numerical
  // instabilities.
  absl::Status ComputeBestNewhallApproximation(
      Instant const& time,
      std::vector<Position<Frame>> const& q,
      std::vector<Velocity<Frame>> const& v) REQUIRES(lock_);

  // Returns an iterator to the polynomial applicable for the given `time`, or
  // `begin` if `time` is before the first polynomial or `end` if `time` is
  // after the last polynomial.  If `time` is the `t_max` of some polynomial,
  // that polynomial is returned.  Time complexity is O(N Log N).
  typename InstantPolynomialPairs::const_iterator
  FindPolynomialForInstantLocked(Instant const& time) const
      REQUIRES_SHARED(lock_);

  // Construction parameters;
  Time const step_;
  Length const tolerance_;
  not_null<
      std::unique_ptr<Checkpointer<serialization::ContinuousTrajectory>>>
      checkpointer_;

  mutable absl::Mutex lock_;

  // Initially set to the construction parameters, and then adjusted when we
  // choose the degree.
  Length adjusted_tolerance_ GUARDED_BY(lock_);
  bool is_unstable_ GUARDED_BY(lock_);

  // The degree of the approximation and its age in number of Newhall
  // approximations.
  int degree_ GUARDED_BY(lock_);
  int degree_age_ GUARDED_BY(lock_);

  // The polynomials are in increasing time order.
  InstantPolynomialPairs polynomials_ GUARDED_BY(lock_);
  Policy polynomial_evaluator_policy_;

  // Lookups into `polynomials_` are expensive because they entail a binary
  // search into a vector that grows over time.  In benchmarks, this can be as
  // costly as the polynomial evaluation itself.  The accesses are not random,
  // though, they are clustered in time and (slowly) increasing.  To take
  // advantage of this, we keep track of the index of the last accessed
  // polynomial and first try to see if the new lookup is for the same
  // polynomial.  This makes us O(1) instead of O(Log N) most of the time and it
  // speeds up the lookup by a factor of 7.  This member is mutable to maintain
  // the fiction that evaluation has no side effects.  In the presence of
  // multithreading it may be that different threads would want to access
  // polynomials at different indices, but by and large the threads progress in
  // parallel, and benchmarks show that there is no adverse performance effects.
  // Any value in the range of `polynomials_` or 0 is correct.
  mutable std::int64_t last_accessed_polynomial_ GUARDED_BY(lock_) = 0;

  // The time at which this trajectory starts.  Set for a nonempty trajectory.
  std::optional<Instant> first_time_ GUARDED_BY(lock_);

  // The points that have not yet been incorporated in a polynomial.  Nonempty
  // for a nonempty trajectory.
  // `last_points_.begin()->first == polynomials_.back().t_max`
  std::vector<std::pair<Instant, DegreesOfFreedom<Frame>>> last_points_
      GUARDED_BY(lock_);

  friend class TestableContinuousTrajectory<Frame>;
};

}  // namespace internal

using internal::ContinuousTrajectory;

}  // namespace _continuous_trajectory
}  // namespace physics
}  // namespace principia

#include "physics/continuous_trajectory_body.hpp"
