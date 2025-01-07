#pragma once

#include <functional>

#include "geometry/frame.hpp"
#include "geometry/permutation.hpp"
#include "ksp_plugin/manœuvre.hpp"
#include "physics/reference_frame.hpp"
#include "physics/rigid_reference_frame.hpp"

namespace principia {
namespace ksp_plugin {
namespace _frames {
namespace internal {

using namespace principia::geometry::_frame;
using namespace principia::geometry::_permutation;
using namespace principia::ksp_plugin::_manœuvre;
using namespace principia::physics::_reference_frame;
using namespace principia::physics::_rigid_reference_frame;

// Thanks to KSP's madness, the reference frame of the celestial body orbited by
// the active vessel, occasionally rotating with its surface, occasionally
// nonrotating.
// The basis is that of Unity's "world space".  The origin is the ineffable
// origin of Unity's "world space".
using World = Frame<serialization::Frame::PluginTag,
                    Arbitrary,
                    Handedness::Left,
                    serialization::Frame::WORLD>;

// Same as `World` but with the y and z axes switched through the looking-glass:
// it is a right-handed basis. "We're all mad here. I'm mad. You're mad."
using AliceWorld = Frame<serialization::Frame::PluginTag,
                         Arbitrary,
                         Handedness::Right,
                         serialization::Frame::ALICE_WORLD>;

// The barycentric reference frame of the solar system.
using Barycentric = Frame<serialization::Frame::PluginTag,
                          Inertial,
                          Handedness::Right,
                          serialization::Frame::BARYCENTRIC>;

// The `Apparent...` frames are used for data obtained after the physics
// simulation of the game has run, and before we perform our correction.

// `World` coordinates from the game, but before the correction.
using ApparentWorld = Frame<serialization::Frame::PluginTag,
                            Arbitrary,
                            Handedness::Left,
                            serialization::Frame::APPARENT_WORLD>;

// The axes are those of `Barycentric`.  The origin is that of `ApparentWorld`,
// and should not be depended upon.
using Apparent = Frame<serialization::Frame::PluginTag,
                       NonRotating,
                       Handedness::Right,
                       serialization::Frame::APPARENT>;

// `Barycentric`, with its y and z axes swapped.
using CelestialSphere = Frame<serialization::Frame::PluginTag,
                              Inertial,
                              Handedness::Left,
                              serialization::Frame::CELESTIAL_SPHERE>;

// The surface frame of a celestial, with the x axis pointing to the origin of
// latitude and longitude, the y axis pointing to the pole with positive
// latitude, and the z axis oriented to form a left-handed basis.
using BodyWorld = Frame<serialization::Frame::PluginTag,
                        Arbitrary,
                        Handedness::Left,
                        serialization::Frame::BODY_WORLD>;

// The frame used for the navball.  Its definition depends on the choice of a
// subclass of FrameField.
using Navball = Frame<serialization::Frame::PluginTag,
                      Arbitrary,
                      Handedness::Left,
                      serialization::Frame::NAVBALL>;

// The frame used for trajectory plotting and manœuvre planning.  Its definition
// depends on the choice of a subclass of RigidReferenceFrame.
using Navigation = Frame<serialization::Frame::PluginTag,
                         Arbitrary,
                         Handedness::Right,
                         serialization::Frame::NAVIGATION>;

// The plotting frame, but with the y and z axes swapped compared to
// `Navigation`.  This frame defines the camera horizontal, and its angular
// velocity defines the angular velocity of the camera (note that the linear
// motion of the camera is defined in-game by following a specific target, which
// may be in motion with respect to `CameraReference`, so the camera is not
// necessarily at rest in that frame).
using CameraReference = Frame<serialization::Frame::PluginTag,
                              Arbitrary,
                              Handedness::Left,
                              serialization::Frame::CAMERA_REFERENCE>;

// `CameraReference`, rotated about its y axis by the angle of the planetarium
// rotation.  KSP compensates for the planetarium rotation so that the
// orientation of the camera remains inertially fixed regardless of whether
// World is rotating; we must undo this compensation in order for the camera to
// be fixed in `CameraReference`.
using CameraCompensatedReference =
    Frame<serialization::Frame::PluginTag,
          Arbitrary,
          Handedness::Left,
          serialization::Frame::CAMERA_COMPENSATED_REFERENCE>;

// A nonrotating referencence frame comoving with the sun with the same axes as
// `AliceWorld`. Since it is nonrotating (though not inertial), differences
// between velocities are consistent with those in an inertial reference frame.
// When `AliceWorld` rotates the axes are not fixed in the reference frame, so
// this (frame, basis) pair is inconsistent across instants. Operations should
// only be performed between simultaneous quantities, then converted to a
// consistent (frame, basis) pair before use.
using AliceSun = Frame<serialization::Frame::PluginTag,
                       NonRotating,
                       Handedness::Right,
                       serialization::Frame::ALICE_SUN>;

// Same as above, but with same axes as `World` instead of those of
// `AliceWorld`. The caveats are the same as for `AliceSun`.
using WorldSun = Frame<serialization::Frame::PluginTag,
                       NonRotating,
                       Handedness::Left,
                       serialization::Frame::WORLD_SUN>;

// Used to identify coordinates in the projective plane.  This is *not* the
// OpenGL camera (which is right-handed) but the Unity camera.
using Camera = Frame<serialization::Frame::PluginTag,
                     Arbitrary,
                     Handedness::Left,
                     serialization::Frame::CAMERA>;

// The origin and axes are those of the KSP part; this defines the position and
// orientation of the part in-game.
using EccentricPart = Frame<serialization::Frame::PluginTag,
                            Arbitrary,
                            Handedness::Left,
                            serialization::Frame::ECCENTRIC_PART>;

// The axes are those of `EccentricPart`.  The origin is the centre of mass of
// the part, which may be offset from `EccentricPart::origin`.
using RigidPart = Frame<serialization::Frame::PluginTag,
                        Arbitrary,
                        Handedness::Left,
                        serialization::Frame::RIGID_PART>;

// The body-centred non-rotating frame for the current main body.
using MainBodyCentred = Frame<serialization::Frame::PluginTag,
                              NonRotating,
                              Handedness::Right,
                              serialization::Frame::MAIN_BODY_CENTRED>;

// Convenient instances of types from `physics` for the above frames.
using NavigationFrame = RigidReferenceFrame<Barycentric, Navigation>;
using NavigationManœuvre = Manœuvre<Barycentric, Navigation>;
using PlottingFrame = ReferenceFrame<Barycentric, Navigation>;

// The map between the vector spaces of `WorldSun` and `AliceSun`.
Permutation<WorldSun, AliceSun> const sun_looking_glass(
    Permutation<WorldSun, AliceSun>::CoordinatePermutation::XZY);

}  // namespace internal

using internal::AliceSun;
using internal::AliceWorld;
using internal::Apparent;
using internal::ApparentWorld;
using internal::Barycentric;
using internal::BodyWorld;
using internal::Camera;
using internal::CameraCompensatedReference;
using internal::CameraReference;
using internal::CelestialSphere;
using internal::EccentricPart;
using internal::MainBodyCentred;
using internal::Navball;
using internal::Navigation;
using internal::NavigationFrame;
using internal::NavigationManœuvre;
using internal::PlottingFrame;
using internal::RigidPart;
using internal::World;
using internal::WorldSun;
using internal::sun_looking_glass;

}  // namespace _frames
}  // namespace ksp_plugin
}  // namespace principia
