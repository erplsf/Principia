#pragma once

#include <functional>
#include <future>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <string>

#include "absl/status/status.h"
#include "absl/synchronization/mutex.h"
#include "base/not_null.hpp"
#include "geometry/frame.hpp"
#include "geometry/grassmann.hpp"
#include "geometry/instant.hpp"
#include "geometry/space_transformations.hpp"
#include "integrators/integrators.hpp"
#include "ksp_plugin/frames.hpp"
#include "ksp_plugin/identification.hpp"
#include "physics/degrees_of_freedom.hpp"
#include "physics/discrete_trajectory.hpp"
#include "physics/discrete_trajectory_segment_iterator.hpp"
#include "physics/ephemeris.hpp"
#include "physics/euler_solver.hpp"
#include "physics/massless_body.hpp"
#include "physics/mechanical_system.hpp"
#include "physics/rigid_motion.hpp"
#include "physics/tensors.hpp"
#include "quantities/named_quantities.hpp"
#include "quantities/quantities.hpp"
#include "serialization/ksp_plugin.pb.h"

namespace principia {
namespace ksp_plugin {

FORWARD_DECLARE(class, Part, FROM(part), INTO(pile_up));

class TestablePileUp;

namespace _pile_up {
namespace internal {

using namespace principia::base::_not_null;
using namespace principia::geometry::_frame;
using namespace principia::geometry::_grassmann;
using namespace principia::geometry::_instant;
using namespace principia::geometry::_space_transformations;
using namespace principia::integrators::_integrators;
using namespace principia::ksp_plugin::_frames;
using namespace principia::ksp_plugin::_identification;
using namespace principia::physics::_degrees_of_freedom;
using namespace principia::physics::_discrete_trajectory;
using namespace principia::physics::_discrete_trajectory_segment_iterator;
using namespace principia::physics::_ephemeris;
using namespace principia::physics::_euler_solver;
using namespace principia::physics::_massless_body;
using namespace principia::physics::_mechanical_system;
using namespace principia::physics::_rigid_motion;
using namespace principia::physics::_tensors;
using namespace principia::quantities::_named_quantities;
using namespace principia::quantities::_quantities;

// The axes are those of Barycentric. The origin is the centre of mass of the
// pile up.  This frame is distinguished from NonRotatingPileUp in that it is
// used to hold uncorrected (apparent) coordinates given by the game, before
// the enforcement of conservation laws; see also Apparent.
using ApparentPileUp = Frame<struct ApparentPileUpTag,
                             NonRotating,
                             Handedness::Right>;

// The origin of `NonRotatingPileUp` is the centre of mass of the pile up.
// Its axes are those of `Barycentric`. It is used to describe the rotational
// motion of the pile up (being a nonrotating frame) without running into
// numerical issues from having a faraway origin like that of `Barycentric`.
// This also makes the quantities more conceptually convenient: the angular
// momentum and inertia tensor with respect to the centre of mass are easier to
// reason with than the same quantities with respect to the barycentre of the
// solar system.
using NonRotatingPileUp = Frame<serialization::Frame::PluginTag,
                                NonRotating,
                                Handedness::Right,
                                serialization::Frame::NON_ROTATING_PILE_UP>;

// The origin of `PileUpPrincipalAxes` is the centre of mass of the pile up. Its
// axes are instantaneous principal axes of the pile up.
using PileUpPrincipalAxes = Frame<serialization::Frame::PluginTag,
                                  Arbitrary,
                                  Handedness::Right,
                                  serialization::Frame::PILE_UP_PRINCIPAL_AXES>;

// A `PileUp` handles a connected component of the graph of `Parts` under
// physical contact.  It advances the history and psychohistory of its component
// `Parts`, modeling them as a massless body at their centre of mass.
class PileUp {
 public:
  PileUp(
      std::list<not_null<Part*>> parts,
      Instant const& t,
      Ephemeris<Barycentric>::AdaptiveStepParameters adaptive_step_parameters,
      Ephemeris<Barycentric>::FixedStepParameters fixed_step_parameters,
      not_null<Ephemeris<Barycentric>*> ephemeris,
      std::function<void()> deletion_callback);

  // Runs the `deletion_callback` passed at construction, if not null.
  virtual ~PileUp();

  std::list<not_null<Part*>> const& parts() const;
  Ephemeris<Barycentric>::FixedStepParameters const& fixed_step_parameters()
      const;

  // Set the rigid motion for the given `part`.  This rigid motion is *apparent*
  // in the sense that it was reported by the game but we know better since we
  // are doing science.
  void SetPartApparentRigidMotion(
      not_null<Part*> part,
      RigidMotion<RigidPart, Apparent> const& rigid_motion);

  // Deforms the pile-up, advances the time, and nudges the parts, in sequence.
  // Does nothing if the psychohistory is already advanced beyond `t`.  Several
  // executions of this method may happen concurrently on multiple threads, but
  // not concurrently with any other method of this class.
  absl::Status DeformAndAdvanceTime(Instant const& t);

  // Recomputes the state of motion of the pile-up based on that of its parts.
  void RecomputeFromParts();

  using PileUpForSerializationIndex =
      std::function<not_null<std::shared_ptr<PileUp>>(int)>;
  using SerializationIndexForPileUp =
      std::function<int(not_null<PileUp const*>)>;

  void WriteToMessage(not_null<serialization::PileUp*> message) const;
  static not_null<std::unique_ptr<PileUp>> ReadFromMessage(
      serialization::PileUp const& message,
      std::function<not_null<Part*>(PartId)> const& part_id_to_part,
      not_null<Ephemeris<Barycentric>*> ephemeris,
      std::function<void()> deletion_callback);

 private:
  // A pointer to a member function of `Part` used to append a point to either
  // trajectory (history or psychohistory).
  using AppendToPartTrajectory =
      void (Part::*)(Instant const&, DegreesOfFreedom<Barycentric> const&);

  // For deserialization.  The iterators are optional for compatibility with
  // old saves.
  PileUp(
      std::list<not_null<Part*>>&& parts,
      Ephemeris<Barycentric>::AdaptiveStepParameters adaptive_step_parameters,
      Ephemeris<Barycentric>::FixedStepParameters fixed_step_parameters,
      DiscreteTrajectory<Barycentric> trajectory,
      std::optional<DiscreteTrajectorySegmentIterator<Barycentric>> history,
      std::optional<DiscreteTrajectorySegmentIterator<Barycentric>>
          psychohistory,
      Bivector<AngularMomentum, NonRotatingPileUp> const& angular_momentum,
      not_null<Ephemeris<Barycentric>*> ephemeris,
      std::function<void()> deletion_callback);

  // Sets `euler_solver_` and updates `rigid_pile_up_`.
  void MakeEulerSolver(InertiaTensor<NonRotatingPileUp> const& inertia_tensor,
                       Instant const& t);

  // Update the degrees of freedom (in `NonRotatingPileUp`) of all the parts by
  // translating the *apparent* degrees of freedom so that their centre of mass
  // matches that computed by integration.
  // `SetPartApparentDegreesOfFreedom` must have been called for each part in
  // the pile-up, or for none.
  // The degrees of freedom set by this method are used by `NudgeParts`.
  void DeformPileUpIfNeeded(Instant const& t);

  // Flows the history authoritatively as far as possible up to `t`, advances
  // the histories of the parts and updates the degrees of freedom of the parts
  // if the pile-up is in the bubble.  After this call, the tail (of `*this`)
  // and of its parts have a (possibly ahistorical) final point exactly at `t`.
  absl::Status AdvanceTime(Instant const& t);

  // Adjusts the degrees of freedom of all parts in this pile up based on the
  // degrees of freedom of the pile-up computed by `AdvanceTime` and on the
  // `NonRotatingPileUp` degrees of freedom of the parts, as set by
  // `DeformPileUpIfNeeded`.
  void NudgeParts() const;

  template<AppendToPartTrajectory append_to_part_trajectory>
  void AppendToPart(DiscreteTrajectory<Barycentric>::iterator it) const;

  // Wrapped in a `unique_ptr` to be moveable.
  not_null<std::unique_ptr<absl::Mutex>> lock_;

  std::list<not_null<Part*>> parts_;
  not_null<Ephemeris<Barycentric>*> ephemeris_;
  Ephemeris<Barycentric>::AdaptiveStepParameters adaptive_step_parameters_;
  Ephemeris<Barycentric>::FixedStepParameters fixed_step_parameters_;

  // Recomputed by the parts subset on every change.  Not serialized.
  Mass mass_;
  Vector<Force, Barycentric> intrinsic_force_;
  Bivector<Torque, NonRotatingPileUp> intrinsic_torque_;
  // The angular momentum change arising from mass loss (or, more generally,
  // mass changes); consistently with the native behaviour of the game, we
  // assume that lost mass carries angular momentum in such a way that the
  // angular velocity of a part remains constant.
  Bivector<AngularMomentum, NonRotatingPileUp> angular_momentum_change_;

  // The trajectory of the pile-up, composed of (at most) two segments, the
  // history and the psychohistory.
  DiscreteTrajectory<Barycentric> trajectory_;

  // The `history_` is the past trajectory of the pile-up.  It is normally
  // integrated with a fixed step using `fixed_instance_`, except in the
  // presence of intrinsic acceleration.  It is authoritative in the sense that
  // it is never going to change.
  DiscreteTrajectorySegmentIterator<Barycentric> history_;

  // The `psychohistory_` is the recent past trajectory of the pile-up.  Since
  // we need to draw something between the last point of the `history_` and the
  // current time, we must have a bit of trajectory that may not cover an entire
  // fixed step.  This part is the `psychohistory_`, and it is forked at the end
  // of the `history_`.  It is not authoritative in the sense that it may not
  // match the `history_` that we'll ultimately compute.  The name comes from
  // the fact that we are trying to predict the future, but since we are not as
  // good as Hari Seldon we only do it over a short period of time.
  DiscreteTrajectorySegmentIterator<Barycentric> psychohistory_;

  // The angular momentum of the pile up with respect to its centre of mass.
  Bivector<AngularMomentum, NonRotatingPileUp> angular_momentum_;

  // When present, this instance is used to integrate the trajectory of this
  // pile-up using a fixed-step integrator.  This instance is destroyed
  // if a variable-step integrator needs to be used because of an intrinsic
  // acceleration.
  std::unique_ptr<typename Integrator<
      Ephemeris<Barycentric>::NewtonianMotionEquation>::Instance>
      fixed_instance_;

  PartTo<RigidMotion<RigidPart, NonRotatingPileUp>> actual_part_rigid_motion_;
  PartTo<RigidMotion<RigidPart, Apparent>> apparent_part_rigid_motion_;

  PartTo<RigidTransformation<RigidPart, PileUpPrincipalAxes>> rigid_pile_up_;
  std::optional<EulerSolver<NonRotatingPileUp, PileUpPrincipalAxes>>
      euler_solver_;

  // Called in the destructor.
  std::function<void()> deletion_callback_;

  friend class ksp_plugin::TestablePileUp;
};

// A convenient data object to track a pile-up and the result of integrating it.
struct PileUpFuture {
  PileUpFuture(not_null<PileUp const*> pile_up,
               std::future<absl::Status> future);
  not_null<PileUp const*> pile_up;
  std::future<absl::Status> future;
};

}  // namespace internal

using internal::ApparentPileUp;
using internal::NonRotatingPileUp;
using internal::PileUp;
using internal::PileUpFuture;
using internal::PileUpPrincipalAxes;

}  // namespace _pile_up
}  // namespace ksp_plugin
}  // namespace principia
