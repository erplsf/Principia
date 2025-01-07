#pragma once

#include <memory>
#include <optional>
#include <string>

#include "gmock/gmock.h"
#include "ksp_plugin/plugin.hpp"  // 🧙 For Plugin.

namespace principia {
namespace ksp_plugin {
namespace _plugin {
namespace internal {

class MockPlugin : public Plugin {
 public:
  MockPlugin() : Plugin("JD2451545", "JD2451545", Angle()) {}
  MockPlugin(MockPlugin const&) = delete;
  MockPlugin(MockPlugin&&) = delete;

  MOCK_METHOD(
      void,
      InsertCelestialAbsoluteCartesian,
      (Index celestial_index,
       std::optional<Index> const& parent_index,
       serialization::GravityModel::Body const& gravity_model,
       serialization::InitialState::Cartesian::Body const& initial_state),
      (override));

  MOCK_METHOD(void, EndInitialization, (), (override));

  MOCK_METHOD(bool,
              HasEncounteredApocalypse,
              (std::string * details),
              (const, override));

  MOCK_METHOD(void,
              UpdateCelestialHierarchy,
              (Index celestial_index, Index parent_index),
              (const, override));

  MOCK_METHOD(Index,
              CelestialIndexOfBody,
              (MassiveBody const& body),
              (const, override));

  MOCK_METHOD(void,
              InsertOrKeepVessel,
              (GUID const& vessel_guid,
               std::string const& vessel_name,
               Index parent_index,
               bool loaded,
               bool& inserted),
              (override));

  MOCK_METHOD(void,
              InsertUnloadedPart,
              (PartId part_id,
               std::string const& name,
               GUID const& vessel_guid,
               RelativeDegreesOfFreedom<AliceSun> const& from_parent),
              (override));

  MOCK_METHOD(void,
              AdvanceTime,
              (Instant const& t, Angle const& planetarium_rotation),
              (override));

  MOCK_METHOD(RelativeDegreesOfFreedom<AliceSun>,
              VesselFromParent,
              (Index parent_index, GUID const& vessel_guid),
              (const, override));

  MOCK_METHOD(RelativeDegreesOfFreedom<AliceSun>,
              CelestialFromParent,
              (Index celestial_index),
              (const, override));

  MOCK_METHOD(void,
              CreateFlightPlan,
              (GUID const& vessel_guid,
               Instant const& final_time,
               Mass const& initial_mass),
              (const, override));

  MOCK_METHOD(void,
              ExtendPredictionForFlightPlan,
              (GUID const& vessel_guid),
              (const, override));

  MOCK_METHOD(void,
              SetPredictionAdaptiveStepParameters,
              (GUID const& vessel_guid,
               Ephemeris<Barycentric>::AdaptiveStepParameters const&
                   prediction_adaptive_step_parameters),
              (const, override));

  MOCK_METHOD(bool, HasVessel, (GUID const& vessel_guid), (const, override));
  MOCK_METHOD(not_null<Vessel*>,
              GetVessel,
              (GUID const& vessel_guid),
              (const, override));

  MOCK_METHOD(not_null<std::unique_ptr<Planetarium>>,
              NewPlanetarium,
              (Planetarium::Parameters const& parameters,
               (Perspective<Navigation, Camera> const& perspective),
               std::function<ScaledSpacePoint(Position<Navigation> const&)>
                   plotting_to_scaled_space),
              (const, override));
  MOCK_METHOD(not_null<std::unique_ptr<NavigationFrame>>,
              NewBodyCentredNonRotatingNavigationFrame,
              (Index reference_body_index),
              (const, override));
  MOCK_METHOD(not_null<std::unique_ptr<NavigationFrame>>,
              NewBarycentricRotatingNavigationFrame,
              (Index primary_index, Index secondary_index),
              (const, override));

  MOCK_METHOD((std::unique_ptr<FrameField<World, Navball>>),
              NavballFrameField,
              (Position<World> const& sun_world_position),
              (const, override));

  MOCK_METHOD((Vector<double, World>),
              VesselTangent,
              (GUID const& vessel_guid),
              (const, override));

  MOCK_METHOD((Vector<double, World>),
              VesselNormal,
              (GUID const& vessel_guid),
              (const, override));

  MOCK_METHOD((Vector<double, World>),
              VesselBinormal,
              (GUID const& vessel_guid),
              (const, override));

  MOCK_METHOD(Velocity<World>,
              VesselVelocity,
              (GUID const& vessel_guid),
              (const, override));

  MOCK_METHOD(Instant, CurrentTime, (), (const, override));

  MOCK_METHOD((Rotation<Barycentric, AliceSun> const&),
              PlanetariumRotation,
              (),
              (const, override));

  MOCK_METHOD(Renderer&, renderer, (), (override));
  MOCK_METHOD(Renderer const&, renderer, (), (const, override));

  MOCK_METHOD(void,
              WriteToMessage,
              (not_null<serialization::Plugin*> message),
              (const, override));
};

}  // namespace internal

using internal::MockPlugin;

}  // namespace _plugin
}  // namespace ksp_plugin
}  // namespace principia
