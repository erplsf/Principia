
#pragma once

#include "base/not_null.hpp"
#include "geometry/named_quantities.hpp"
#include "geometry/orthogonal_map.hpp"
#include "geometry/perspective.hpp"
#include "geometry/rp2_point.hpp"
#include "geometry/sphere.hpp"
#include "ksp_plugin/frames.hpp"
#include "physics/degrees_of_freedom.hpp"
#include "physics/discrete_trajectory.hpp"
#include "physics/rigid_motion.hpp"
#include "physics/trajectory.hpp"
#include "quantities/quantities.hpp"

namespace principia {
namespace ksp_plugin {
namespace internal_planetarium {

using base::not_null;
using geometry::Instant;
using geometry::OrthogonalMap;
using geometry::Perspective;
using geometry::RP2Point;
using geometry::Sphere;
using physics::DegreesOfFreedom;
using physics::DiscreteTrajectory;
using physics::RigidMotion;
using physics::Trajectory;
using quantities::Length;

// A planetarium is a system of spheres together with a perspective.  In this
// setting it is possible to draw trajectories in the projective plane.
class Planetarium final {
 public:
  // TODO(phl): All this Navigation is weird.  Should it be named Plotting?
  // In particular Navigration vs. NavigationFrame is a mess.
  // TODO(phl): Maybe replace the spheres with an ephemeris.
  Planetarium(std::vector<Sphere<Length, Barycentric>> const& spheres,
              Perspective<Navigation, Camera, Length, OrthogonalMap> const&
                  perspective,
              not_null<NavigationFrame*> plotting_frame);

  // A no-op method that just returns all the points in the |trajectory|.
  std::vector<RP2Point<Length, Camera>> PlotMethod0(
      DiscreteTrajectory<Barycentric> const& trajectory,
      Instant const& now) const;

  // A na�ve method that doesn't pay any attention to the perspective but tries
  // to ensure that the points before the perspective are separated by less than
  // |tolerance|.
  std::vector<RP2Point<Length, Camera>> PlotMethod1(
      Trajectory<Barycentric> const& trajectory,
      Instant const& now,
      Length const& tolerance) const;

 private:
  // Computes the coordinates of the |spheres_| in the |plotting_frame_| at time
  // |now|.
  std::vector<Sphere<Length, Navigation>> ComputePlottableSpheres(
      Instant const& now) const;

  // Appends to |rp2_points| a point corresponding to the
  // |barycentric_degrees_of_freedom| transformed in the |plotting_frame_| at
  // time |t|, but only if that point is not hidden by a sphere.
  void AppendRP2PointIfNeeded(
      Instant const& t,
      DegreesOfFreedom<Barycentric> const& barycentric_degrees_of_freedom,
      std::vector<Sphere<Length, Navigation>> const& plottable_spheres,
      std::vector<RP2Point<Length, Camera>>& rp2_points) const;

  std::vector<Sphere<Length, Barycentric>> const spheres_;
  Perspective<Navigation, Camera, Length, OrthogonalMap> const
      perspective_;
  not_null<NavigationFrame*> const plotting_frame_;
};

}  // namespace internal_planetarium

using internal_planetarium::Planetarium;

}  // namespace ksp_plugin
}  // namespace principia
