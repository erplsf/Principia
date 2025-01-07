#pragma once

#include "physics/mechanical_system.hpp"

#include "geometry/space.hpp"
#include "geometry/space_transformations.hpp"
#include "quantities/si.hpp"

namespace principia {
namespace physics {
namespace _mechanical_system {
namespace internal {

using namespace principia::geometry::_space;
using namespace principia::geometry::_space_transformations;
using namespace principia::quantities::_si;

template<typename InertialFrame, typename SystemFrame>
template<typename BodyFrame>
void MechanicalSystem<InertialFrame, SystemFrame>::AddRigidBody(
    RigidMotion<BodyFrame, InertialFrame> const& motion,
    Mass const& mass,
    _tensors::InertiaTensor<BodyFrame> const& tensors) {
  DegreesOfFreedom<InertialFrame> const degrees_of_freedom =
      motion({BodyFrame::origin, BodyFrame::unmoving});
  SymmetricBilinearForm<MomentOfInertia, InertialFrame, Vector> const
      inertia_tensor_in_inertial_axes =
          motion.orthogonal_map()(tensors.AnticommutatorInverse());
  centre_of_mass_.Add(degrees_of_freedom, mass);
  body_linear_motions_.emplace_back(degrees_of_freedom, mass);
  sum_of_inertia_tensors_ += inertia_tensor_in_inertial_axes;
  sum_of_intrinsic_angular_momenta_ +=
      Anticommutator(inertia_tensor_in_inertial_axes,
                     motion.template angular_velocity_of<BodyFrame>());
}

template<typename InertialFrame, typename SystemFrame>
RigidMotion<SystemFrame, InertialFrame>
MechanicalSystem<InertialFrame, SystemFrame>::LinearMotion() const {
  DegreesOfFreedom<InertialFrame> const centre_of_mass = centre_of_mass_.Get();
  return RigidMotion<SystemFrame, InertialFrame>(
      RigidTransformation<SystemFrame, InertialFrame>(
          SystemFrame::origin,
          centre_of_mass.position(),
          OrthogonalMap<SystemFrame, InertialFrame>::Identity()),
      InertialFrame::nonrotating,
      centre_of_mass.velocity());
}

template<typename InertialFrame, typename SystemFrame>
Mass const& MechanicalSystem<InertialFrame, SystemFrame>::mass() const {
  return centre_of_mass_.weight();
}

template<typename InertialFrame, typename SystemFrame>
DegreesOfFreedom<InertialFrame>
MechanicalSystem<InertialFrame, SystemFrame>::centre_of_mass() const {
  return centre_of_mass_.Get();
}

template<typename InertialFrame, typename SystemFrame>
Bivector<AngularMomentum, SystemFrame>
MechanicalSystem<InertialFrame, SystemFrame>::AngularMomentum() const {
  RigidMotion<InertialFrame, SystemFrame> const to_system_frame =
      LinearMotion().Inverse();
  Bivector<quantities::_named_quantities::AngularMomentum, SystemFrame> result =
      to_system_frame.orthogonal_map()(sum_of_intrinsic_angular_momenta_);
  for (auto const& [degrees_of_freedom, m] : body_linear_motions_) {
    DegreesOfFreedom<SystemFrame> const degrees_of_freedom_in_system_frame =
        to_system_frame(degrees_of_freedom);
    Displacement<SystemFrame> const r =
        degrees_of_freedom_in_system_frame.position() - SystemFrame::origin;
    Velocity<SystemFrame> const v =
        degrees_of_freedom_in_system_frame.velocity();
    Vector<Momentum, SystemFrame> const p = m * v;
    result += Wedge(r, p) * Radian;
  }
  return result;
}

template<typename InertialFrame, typename SystemFrame>
InertiaTensor<SystemFrame>
MechanicalSystem<InertialFrame, SystemFrame>::InertiaTensor() const {
  RigidMotion<InertialFrame, SystemFrame> const to_system_frame =
      LinearMotion().Inverse();
  SymmetricBilinearForm<MomentOfInertia, SystemFrame, Vector> result =
      to_system_frame.orthogonal_map()(sum_of_inertia_tensors_);
  for (auto const& [degrees_of_freedom, m] : body_linear_motions_) {
    DegreesOfFreedom<SystemFrame> const degrees_of_freedom_in_system_frame =
        to_system_frame(degrees_of_freedom);
    Displacement<SystemFrame> const r =
        degrees_of_freedom_in_system_frame.position() - SystemFrame::origin;
    result += m * SymmetricSquare(r);
  }
  return result.Anticommutator();
}

}  // namespace internal
}  // namespace _mechanical_system
}  // namespace physics
}  // namespace principia
