#pragma once

#include "physics/barycentric_rotating_reference_frame.hpp"

#include <algorithm>
#include <utility>

#include "geometry/barycentre_calculator.hpp"
#include "geometry/r3x3_matrix.hpp"
#include "quantities/quantities.hpp"
#include "quantities/si.hpp"

namespace principia {
namespace physics {
namespace _barycentric_rotating_reference_frame {
namespace internal {

using namespace principia::geometry::_barycentre_calculator;
using namespace principia::geometry::_grassmann;
using namespace principia::geometry::_orthogonal_map;
using namespace principia::geometry::_r3x3_matrix;
using namespace principia::quantities::_elementary_functions;
using namespace principia::quantities::_named_quantities;
using namespace principia::quantities::_quantities;
using namespace principia::quantities::_si;

template<typename InertialFrame, typename ThisFrame>
BarycentricRotatingReferenceFrame<InertialFrame, ThisFrame>::
BarycentricRotatingReferenceFrame(
    not_null<Ephemeris<InertialFrame> const*> ephemeris,
    not_null<MassiveBody const*> primary,
    not_null<MassiveBody const*> secondary)
    : ephemeris_(std::move(ephemeris)),
      primary_(std::move(primary)),
      secondary_(std::move(secondary)),
      primary_trajectory_(ephemeris_->trajectory(primary_)),
      secondary_trajectory_(ephemeris_->trajectory(secondary_)) {}

template<typename InertialFrame, typename ThisFrame>
not_null<MassiveBody const*>
BarycentricRotatingReferenceFrame<InertialFrame, ThisFrame>::primary() const {
  return primary_;
}

template<typename InertialFrame, typename ThisFrame>
not_null<MassiveBody const*>
BarycentricRotatingReferenceFrame<InertialFrame, ThisFrame>::secondary() const {
  return secondary_;
}

template<typename InertialFrame, typename ThisFrame>
Instant BarycentricRotatingReferenceFrame<InertialFrame, ThisFrame>::t_min()
    const {
  return std::max(primary_trajectory_->t_min(), secondary_trajectory_->t_min());
}

template<typename InertialFrame, typename ThisFrame>
Instant BarycentricRotatingReferenceFrame<InertialFrame, ThisFrame>::t_max()
    const {
  return std::min(primary_trajectory_->t_max(), secondary_trajectory_->t_max());
}

template<typename InertialFrame, typename ThisFrame>
RigidMotion<InertialFrame, ThisFrame>
BarycentricRotatingReferenceFrame<InertialFrame, ThisFrame>::ToThisFrameAtTime(
    Instant const& t) const {
  DegreesOfFreedom<InertialFrame> const primary_degrees_of_freedom =
      primary_trajectory_->EvaluateDegreesOfFreedom(t);
  DegreesOfFreedom<InertialFrame> const secondary_degrees_of_freedom =
      secondary_trajectory_->EvaluateDegreesOfFreedom(t);
  DegreesOfFreedom<InertialFrame> const barycentre_degrees_of_freedom =
      Barycentre<DegreesOfFreedom<InertialFrame>, GravitationalParameter>(
          {primary_degrees_of_freedom,
           secondary_degrees_of_freedom},
          {primary_->gravitational_parameter(),
           secondary_->gravitational_parameter()});

  Rotation<InertialFrame, ThisFrame> rotation =
          Rotation<InertialFrame, ThisFrame>::Identity();
  AngularVelocity<InertialFrame> angular_velocity;
  ComputeAngularDegreesOfFreedom(primary_degrees_of_freedom,
                                 secondary_degrees_of_freedom,
                                 rotation,
                                 angular_velocity);

  RigidTransformation<InertialFrame, ThisFrame> const
      rigid_transformation(barycentre_degrees_of_freedom.position(),
                           ThisFrame::origin,
                           rotation.template Forget<OrthogonalMap>());
  return RigidMotion<InertialFrame, ThisFrame>(
             rigid_transformation,
             angular_velocity,
             barycentre_degrees_of_freedom.velocity());
}

template<typename InertialFrame, typename ThisFrame>
void BarycentricRotatingReferenceFrame<InertialFrame, ThisFrame>::
WriteToMessage(
    not_null<serialization::RigidReferenceFrame*> const message) const {
  auto* const extension = message->MutableExtension(
      serialization::BarycentricRotatingReferenceFrame::extension);
  extension->set_primary(ephemeris_->serialization_index_for_body(primary_));
  extension->set_secondary(
      ephemeris_->serialization_index_for_body(secondary_));
}

template<typename InertialFrame, typename ThisFrame>
not_null<std::unique_ptr<
    BarycentricRotatingReferenceFrame<InertialFrame, ThisFrame>>>
BarycentricRotatingReferenceFrame<InertialFrame, ThisFrame>::ReadFromMessage(
    not_null<Ephemeris<InertialFrame> const*> const ephemeris,
    serialization::BarycentricRotatingReferenceFrame const & message) {
  return std::make_unique<BarycentricRotatingReferenceFrame>(
      ephemeris,
      ephemeris->body_for_serialization_index(message.primary()),
      ephemeris->body_for_serialization_index(message.secondary()));
}

template<typename InertialFrame, typename ThisFrame>
Vector<Acceleration, InertialFrame>
BarycentricRotatingReferenceFrame<InertialFrame, ThisFrame>::
GravitationalAcceleration(Instant const& t,
                          Position<InertialFrame> const& q) const {
  return ephemeris_->ComputeGravitationalAccelerationOnMasslessBody(q, t);
}

template<typename InertialFrame, typename ThisFrame>
SpecificEnergy BarycentricRotatingReferenceFrame<InertialFrame, ThisFrame>::
GravitationalPotential(Instant const& t,
                       Position<InertialFrame> const& q) const {
  return ephemeris_->ComputeGravitationalPotential(q, t);
}

template<typename InertialFrame, typename ThisFrame>
AcceleratedRigidMotion<InertialFrame, ThisFrame>
BarycentricRotatingReferenceFrame<InertialFrame, ThisFrame>::MotionOfThisFrame(
    Instant const& t) const {
  DegreesOfFreedom<InertialFrame> const primary_degrees_of_freedom =
      primary_trajectory_->EvaluateDegreesOfFreedom(t);
  DegreesOfFreedom<InertialFrame> const secondary_degrees_of_freedom =
      secondary_trajectory_->EvaluateDegreesOfFreedom(t);

  Vector<Acceleration, InertialFrame> const primary_acceleration =
      ephemeris_->ComputeGravitationalAccelerationOnMassiveBody(primary_, t);
  Vector<Acceleration, InertialFrame> const secondary_acceleration =
      ephemeris_->ComputeGravitationalAccelerationOnMassiveBody(secondary_, t);

  auto const to_this_frame = ToThisFrameAtTime(t);

  // TODO(egg): TeX and reference.
  RelativeDegreesOfFreedom<InertialFrame> const secondary_primary =
      secondary_degrees_of_freedom - primary_degrees_of_freedom;
  Displacement<InertialFrame> const& r = secondary_primary.displacement();
  Velocity<InertialFrame> const& ṙ = secondary_primary.velocity();
  Vector<Acceleration, InertialFrame> const r̈ =
      secondary_acceleration - primary_acceleration;
  AngularVelocity<InertialFrame> const& ω =
      to_this_frame.template angular_velocity_of<ThisFrame>();
  Variation<AngularVelocity<InertialFrame>> const
      angular_acceleration_of_to_frame =
          (Wedge(r, r̈) * Radian - 2 * ω * InnerProduct(r, ṙ)) / r.Norm²();

  Vector<Acceleration, InertialFrame> const acceleration_of_to_frame_origin =
      Barycentre<Vector<Acceleration, InertialFrame>, GravitationalParameter>(
          {primary_acceleration, secondary_acceleration},
          {primary_->gravitational_parameter(),
           secondary_->gravitational_parameter()});
  return AcceleratedRigidMotion<InertialFrame, ThisFrame>(
             to_this_frame,
             angular_acceleration_of_to_frame,
             acceleration_of_to_frame_origin);
}

template<typename InertialFrame, typename ThisFrame>
void BarycentricRotatingReferenceFrame<InertialFrame, ThisFrame>::
ComputeAngularDegreesOfFreedom(
    DegreesOfFreedom<InertialFrame> const& primary_degrees_of_freedom,
    DegreesOfFreedom<InertialFrame> const& secondary_degrees_of_freedom,
    Rotation<InertialFrame, ThisFrame>& rotation,
    AngularVelocity<InertialFrame>& angular_velocity) {
  RelativeDegreesOfFreedom<InertialFrame> const reference =
      secondary_degrees_of_freedom - primary_degrees_of_freedom;
  Displacement<InertialFrame> const& reference_direction =
      reference.displacement();
  Velocity<InertialFrame> const reference_normal =
      reference.velocity().OrthogonalizationAgainst(reference_direction);
  Bivector<Product<Length, Speed>, InertialFrame> const reference_binormal =
      Wedge(reference_direction, reference_normal);
  rotation = Rotation<InertialFrame, ThisFrame>(Normalize(reference_direction),
                                                Normalize(reference_normal),
                                                Normalize(reference_binormal));
  angular_velocity = reference_binormal * Radian / reference_direction.Norm²();
}

}  // namespace internal
}  // namespace _barycentric_rotating_reference_frame
}  // namespace physics
}  // namespace principia