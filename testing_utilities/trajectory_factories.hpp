﻿#pragma once

#include <memory>

#include "base/not_null.hpp"
#include "geometry/named_quantities.hpp"
#include "physics/discrete_trajectory.hpp"
#include "quantities/named_quantities.hpp"
#include "quantities/quantities.hpp"

namespace principia {
namespace testing_utilities {
namespace internal_trajectory_factories {

using base::not_null;
using geometry::Instant;
using physics::DiscreteTrajectory;
using quantities::AngularFrequency;
using quantities::Length;
using quantities::Time;

// A circular trajectory in the plane XY, centred at the origin.  The first
// point is at time |t1|, the last point at a time < |t2|.
template<typename Frame>
not_null<std::unique_ptr<DiscreteTrajectory<Frame>>> NewCircularTrajectory(
    AngularFrequency const& ω,
    Length const& r,
    Time const& Δt,
    Instant const& t1,
    Instant const& t2);
template<typename Frame>
not_null<std::unique_ptr<DiscreteTrajectory<Frame>>> NewCircularTrajectory(
    Time const& period,
    Length const& r,
    Time const& Δt,
    Instant const& t1,
    Instant const& t2);

template<typename Frame>
void AppendTrajectory(DiscreteTrajectory<Frame> const& from,
                      DiscreteTrajectory<Frame>& to);

}  // namespace internal_trajectory_factories

using internal_trajectory_factories::AppendTrajectory;
using internal_trajectory_factories::NewCircularTrajectory;

}  // namespace testing_utilities
}  // namespace principia

#include "testing_utilities/trajectory_factories_body.hpp"
