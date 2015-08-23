﻿#include "ksp_plugin/plugin.hpp"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "testing_utilities/solar_system.hpp"

namespace principia {

using quantities::Abs;
using quantities::ArcTan;
using quantities::Cos;
using quantities::Sin;
using quantities::Sqrt;
using si::Day;
using si::Hour;
using si::Minute;
using si::Radian;
using si::AstronomicalUnit;
using testing_utilities::SolarSystem;
using testing_utilities::ICRFJ2000Ecliptic;
using ::testing::AllOf;
using ::testing::Eq;
using ::testing::Ge;
using ::testing::Gt;
using ::testing::Le;
using ::testing::Lt;
using testing_utilities::AbsoluteError;
using testing_utilities::RelativeError;

namespace ksp_plugin {

class PluginIntegrationTest : public testing::Test {
 protected:
  PluginIntegrationTest()
      : icrf_to_barycentric_positions_(ICRFJ2000Ecliptic::origin,
                                       Barycentric::origin,
                                       ircf_to_barycentric_linear_),
        looking_glass_(Permutation<ICRFJ2000Ecliptic, AliceSun>::XZY),
        solar_system_(SolarSystem::AtСпутник1Launch(
                      SolarSystem::Accuracy::kAllBodiesAndOblateness)),
        initial_time_(42 * Second),
        planetarium_rotation_(1 * Radian),
        plugin_(make_not_null_unique<Plugin>(
                    initial_time_,
                    planetarium_rotation_)),
        bodies_(solar_system_->massive_bodies()) {
    sun_gravitational_parameter_ =
        bodies_[SolarSystem::kSun]->gravitational_parameter();
    satellite_initial_displacement_ =
        Displacement<AliceSun>({3111.0 * Kilo(Metre),
                                4400.0 * Kilo(Metre),
                                3810.0 * Kilo(Metre)});
    auto const tangent =
        satellite_initial_displacement_ * Bivector<double, AliceSun>({1, 2, 3});
    Vector<double, AliceSun> const unit_tangent = Normalize(tangent);
    EXPECT_THAT(
        InnerProduct(unit_tangent,
                     satellite_initial_displacement_ /
                         satellite_initial_displacement_.Norm()),
        Eq(0));
    // This yields a circular orbit.
    satellite_initial_velocity_ =
        Sqrt(bodies_[SolarSystem::kEarth]->gravitational_parameter() /
                 satellite_initial_displacement_.Norm()) * unit_tangent;
  }

  DegreesOfFreedom<Barycentric> ICRFToBarycentric(
      DegreesOfFreedom<ICRFJ2000Ecliptic> const& degrees_of_freedom) {
    return {icrf_to_barycentric_positions_(degrees_of_freedom.position()),
            ircf_to_barycentric_linear_(degrees_of_freedom.velocity())};
  }

  void InsertAllSolarSystemBodies() {
    for (std::size_t index = SolarSystem::kSun;
         index < bodies_.size();
         ++index) {
      std::unique_ptr<Index> parent_index =
          index == SolarSystem::kSun
              ? nullptr
              : std::make_unique<Index>(SolarSystem::parent(index));
      DegreesOfFreedom<Barycentric> const initial_state =
          ICRFToBarycentric(solar_system_->trajectories()[index]->
                            last().degrees_of_freedom());
    // barf_cast, to be fixed when we have rvalue conversion.
    plugin_->DirectlyInsertCelestial(
        index,
        parent_index.get(),
        initial_state,
        std::move(
            *reinterpret_cast<std::unique_ptr<MassiveBody>*>(
                &bodies_[index])));
    }
  }

  Identity<ICRFJ2000Ecliptic, Barycentric> ircf_to_barycentric_linear_;
  AffineMap<ICRFJ2000Ecliptic,
            Barycentric,
            Length,
            Identity> icrf_to_barycentric_positions_;
  Permutation<ICRFJ2000Ecliptic, AliceSun> looking_glass_;
  not_null<std::unique_ptr<SolarSystem>> solar_system_;
  Instant initial_time_;
  GravitationalParameter sun_gravitational_parameter_;
  Angle planetarium_rotation_;

  not_null<std::unique_ptr<Plugin>> plugin_;

  // These initial conditions will yield a low circular orbit around Earth.
  Displacement<AliceSun> satellite_initial_displacement_;
  Velocity<AliceSun> satellite_initial_velocity_;

 private:
  SolarSystem::Bodies bodies_;
};

TEST_F(PluginIntegrationTest, AdvanceTimeWithCelestialsOnly) {
  InsertAllSolarSystemBodies();
  plugin_->EndInitialization();
  Time const δt = 0.02 * Second;
  Angle const planetarium_rotation = 42 * Radian;
  // We step for long enough that we will find a new segment.
  Instant t = initial_time_;
  for (t += δt; t < initial_time_ + 10 * 45 * Minute; t += δt) {
    plugin_->AdvanceTime(t, planetarium_rotation);
  }
  EXPECT_THAT(
      RelativeError(
          plugin_->
              CelestialFromParent(SolarSystem::kEarth).displacement().Norm(),
          1 * AstronomicalUnit),
      Lt(0.01));
  serialization::Plugin plugin_message;
  plugin_->WriteToMessage(&plugin_message);
  plugin_ = Plugin::ReadFromMessage(plugin_message);
  // Having saved and loaded, we compute a new segment again, this probably
  // exercises apocalypse-type bugs.
  for (; t < initial_time_ + 20 * 45 * Minute; t += δt) {
    plugin_->AdvanceTime(t, planetarium_rotation);
  }
  EXPECT_THAT(
      RelativeError(
          plugin_->
              CelestialFromParent(SolarSystem::kEarth).displacement().Norm(),
          1 * AstronomicalUnit),
      Lt(0.01));
}

#if 0
// Checks that the plugin correctly advances the history of newly inserted
// vessels with the prolongation integrator (using small steps), then switches
// to the history integrator.  Also tests the removal of vessels.
// The sequence of additions and removals is as follows (using the constants
// defined below; recall that |initial_time_ == HistoryTime(0)|).
// * |HistoryTime(0)|               : Insert |enterprise|.
// * |HistoryTime(0) + a_while|     : Insert |enterprise_d|.
// * |HistoryTime(1) + a_while|     : Insert |stargazer| and |bradbury|.
// * |HistoryTime(1) + half_a_step| : Remove |bradbury| (it is present at
//                                    |HistoryTime(1) + half_a_step|, but not at
//                                    |HistoryTime(1) + half_a_step + δt|).
//                                    It is thus removed while unsynchronized.
// * |HistoryTime(2)|               : Insert |constantinople|.
// * |HistoryTime(3)|               : Remove |enterprise|.
TEST_F(PluginIntegrationTest, AdvanceTimeWithVessels) {
  Time const δt = 1 * Second;
  Time const a_while = 2 * δt;
  Time const half_a_step = 5 * δt;
  Time const ε_δt = 0.1 * δt;
  EXPECT_THAT(half_a_step, Eq(plugin_->Δt() / 2));
  GUID const enterprise = "NCC-1701";
  GUID const enterprise_d = "NCC-1701-D";
  GUID const stargazer = "NCC-2893";
  GUID const bradbury = "NX-72307";
  GUID const constantinople = "NCC-43622";
  InsertAllSolarSystemBodies();
  plugin_->EndInitialization();
  Angle const planetarium_rotation = 42 * Radian;
  std::size_t expected_number_of_new_vessels = 0;
  std::size_t expected_number_of_old_vessels = 0;
  InSequence in_sequence;
  EXPECT_FALSE(plugin_->has_vessel(enterprise));
  Instant sync_time = initial_time_;
  InsertVessel(enterprise,
               &expected_number_of_new_vessels,
               HistoryTime(sync_time, 0));
  EXPECT_TRUE(plugin_->has_vessel(enterprise));
  // Therer are no old vessels, this will make all the new vessels old
  // instantly without performing any syncing computations.
  plugin_->AdvanceTime(HistoryTime(sync_time, 0) + δt, planetarium_rotation);
  expected_number_of_old_vessels = expected_number_of_new_vessels;
  expected_number_of_new_vessels = 0;
  sync_time = HistoryTime(sync_time, 0) + δt;
  for (int step = 0; step < 10; ++step) {
    for (Instant t = HistoryTime(sync_time, step) + 2 * δt;
         t < HistoryTime(sync_time, step + 1);
         t += δt) {
      // Keep our vessels.  Make sure we're not inserting new ones.
      if (step <= 3) {
        KeepVessel(enterprise);
      }
      if (t > HistoryTime(sync_time, 0) + a_while + ε_δt) {
        KeepVessel(enterprise_d);
      }
      if (t > HistoryTime(sync_time, 1) + a_while + ε_δt) {
        KeepVessel(stargazer);
      }
      if (t > HistoryTime(sync_time, 1) + a_while + ε_δt &&
          t < HistoryTime(sync_time, 1) + half_a_step - ε_δt) {
        KeepVessel(bradbury);
      } else if (AbsoluteError(t - HistoryTime(sync_time, 1),
                               half_a_step) < ε_δt) {
        // We will be removing |bradbury| in this step.
        --expected_number_of_new_vessels;
      }
      if (step > 2) {
        KeepVessel(constantinople);
      }
      plugin_->AdvanceTime(t, planetarium_rotation);
      if (AbsoluteError(t - HistoryTime(sync_time, 0), a_while) < ε_δt) {
        InsertVessel(enterprise_d, &expected_number_of_new_vessels, t);
      } else if (AbsoluteError(t - HistoryTime(sync_time, 1), a_while) < ε_δt) {
        InsertVessel(stargazer, &expected_number_of_new_vessels, t);
        InsertVessel(bradbury, &expected_number_of_new_vessels, t);
      }
    }
    // Keep the vessels for the history-advancing step.
    if (step <= 3) {
      KeepVessel(enterprise);
    }
    KeepVessel(enterprise_d);
    if (step >= 1) {
      KeepVessel(stargazer);
    }
    if (step > 2) {
      KeepVessel(constantinople);
    }

    plugin_->AdvanceTime(HistoryTime(sync_time, step + 1) + δt,
                         planetarium_rotation);
    if (step == 2) {
      InsertVessel(constantinople,
                   &expected_number_of_new_vessels,
                   HistoryTime(sync_time, step + 1) + δt);
    } else if (step == 3) {
      // We will be removing |enterprise|.
      --expected_number_of_old_vessels;
    }
  }
}

// Checks that the plugin correctly advances the history of newly inserted
// vessels with the prolongation integrator (using small steps), then switches
// to the history integrator.  Also tests the removal of vessels.
// The sequence of additions and removals is as follows (using the constants
// defined below; recall that |initial_time_ == HistoryTime(0)|).
// * |HistoryTime(0)|               : Insert |enterprise|, insert into bubble
//                                    (1 part).
// * |HistoryTime(0) + a_while|     : Insert |enterprise_d|.
// * |HistoryTime(1) + a_while|     : Insert |enterprise_d| into bubble
//                                    (2 parts).
// * |HistoryTime(1) + half_a_step| : Split |enterprise_d|, yielding
//                                    |enterprise_d_saucer|.
// * |HistoryTime(2)|               : Remove |enterprise|, merge |enterprise_d|
//                                    and |enterprise_d_saucer|.
// * |HistoryTime(2 + half_a_step)| : Physics bubble ends.
TEST_F(PluginIntegrationTest, PhysicsBubble) {
  Time const δt = 1 * Second;
  Time const a_while = 2 * δt;
  Time const half_a_step = 5 * δt;
  Time const ε_δt = 0.1 * δt;
  EXPECT_THAT(half_a_step, Eq(plugin_->Δt() / 2));
  GUID const enterprise = "NCC-1701";
  GUID const enterprise_d = "NCC-1701-D";
  GUID const enterprise_d_saucer = "NCC-1701-D (saucer)";
  GUID const constantinople = "NCC-43622";
  auto const make_enterprise_whole_ship = []() {
    return std::make_pair(
        PartId(0U),
        make_not_null_unique<Part<World>>(
            DegreesOfFreedom<World>(
                World::origin,
                Velocity<World>({1 * Metre / Second,
                                 0 * Metre / Second,
                                 0 * Metre / Second})),
            1 * Kilogram,
            Vector<Acceleration, World>({1 * Metre / Pow<2>(Second),
                                         0 * Metre / Pow<2>(Second),
                                         0 * Metre / Pow<2>(Second)})));
  };
  auto const make_enterprise_d_engineering_section = []() {
    return std::make_pair(
        PartId(1U),
        make_not_null_unique<Part<World>>(
            DegreesOfFreedom<World>(
                World::origin +
                    Displacement<World>({1 * Metre, 0 * Metre, 0 * Metre}),
                Velocity<World>({0 * Metre / Second,
                                 1 * Metre / Second,
                                 0 * Metre / Second})),
            3 * Kilogram,
            Vector<Acceleration, World>({1 * Metre / Pow<2>(Second),
                                         0 * Metre / Pow<2>(Second),
                                         0 * Metre / Pow<2>(Second)})));
  };
  auto const make_enterprise_d_saucer_section = []() {
    return std::make_pair(
        PartId(2U),
        make_not_null_unique<Part<World>>(
            DegreesOfFreedom<World>(
                World::origin -
                    Displacement<World>({1 * Metre, 0 * Metre, 0 * Metre}),
                -Velocity<World>({0 * Metre / Second,
                                  1 * Metre / Second,
                                  0 * Metre / Second})),
            5 * Kilogram,
            Vector<Acceleration, World>({1 * Metre / Pow<2>(Second),
                                         0 * Metre / Pow<2>(Second),
                                         0 * Metre / Pow<2>(Second)})));
  };
  InsertAllSolarSystemBodies();
  plugin_->EndInitialization();
  Angle const planetarium_rotation = 42 * Radian;
  std::size_t expected_number_of_new_off_rails_vessels = 0;
  std::size_t expected_number_of_dirty_old_on_rails_vessels = 0;
  std::size_t expected_number_of_clean_old_vessels = 0;
  std::vector<IdAndOwnedPart> parts;
  bool expect_to_have_physics_bubble = true;
  bool expect_intrinsic_acceleration = false;
  Instant sync_time = initial_time_;
  // The synchronizer vessel.
  InsertVessel(constantinople,
               &expected_number_of_clean_old_vessels,
               HistoryTime(sync_time, 0));
  plugin_->AdvanceTime(HistoryTime(sync_time, 0) + δt,
                       planetarium_rotation);
  sync_time = HistoryTime(sync_time, 0) + δt;

  InsertVessel(enterprise,
               &expected_number_of_new_off_rails_vessels,
               HistoryTime(sync_time, 0));
  --expected_number_of_new_off_rails_vessels;
  for (int step = 0; step < 10; ++step) {
    for (Instant t = HistoryTime(sync_time, step) + 2 * δt;
         t < HistoryTime(sync_time, step + 1);
         t += δt) {
      // Keep the synchronizer.
      KeepVessel(constantinople);
      // Keep our vessels.  Make sure we're not inserting new ones.
      if (t < HistoryTime(sync_time, 0) + a_while + ε_δt) {
        KeepVessel(enterprise);
        parts.clear();
        parts.emplace_back(std::move(make_enterprise_whole_ship()));
        plugin_->AddVesselToNextPhysicsBubble(enterprise, std::move(parts));
      } else if (t > HistoryTime(sync_time, 0) + a_while + ε_δt &&
                 t < HistoryTime(sync_time, 1) + half_a_step + ε_δt) {
        KeepVessel(enterprise_d);
        parts.clear();
        parts.emplace_back(std::move(make_enterprise_d_engineering_section()));
        parts.emplace_back(std::move(make_enterprise_d_saucer_section()));
        plugin_->AddVesselToNextPhysicsBubble(enterprise_d, std::move(parts));
        KeepVessel(enterprise);
        parts.clear();
        parts.emplace_back(std::move(make_enterprise_whole_ship()));
        plugin_->AddVesselToNextPhysicsBubble(enterprise, std::move(parts));
      } else if (t > HistoryTime(sync_time, 1) + half_a_step - ε_δt &&
                 t < HistoryTime(sync_time, 2) + ε_δt) {
        KeepVessel(enterprise_d);
        parts.clear();
        parts.emplace_back(std::move(make_enterprise_d_engineering_section()));
        plugin_->AddVesselToNextPhysicsBubble(enterprise_d, std::move(parts));
        KeepVessel(enterprise_d_saucer);
        parts.clear();
        parts.emplace_back(std::move(make_enterprise_d_saucer_section()));
        plugin_->AddVesselToNextPhysicsBubble(enterprise_d_saucer,
                                              std::move(parts));
        KeepVessel(enterprise);
        parts.clear();
        parts.emplace_back(std::move(make_enterprise_whole_ship()));
        plugin_->AddVesselToNextPhysicsBubble(enterprise, std::move(parts));
      } else if (t > HistoryTime(sync_time, 2) - ε_δt &&
                 t < HistoryTime(sync_time, 2) + half_a_step - ε_δt) {
        KeepVessel(enterprise_d);
        parts.clear();
        parts.emplace_back(std::move(make_enterprise_d_engineering_section()));
        parts.emplace_back(std::move(make_enterprise_d_saucer_section()));
        plugin_->AddVesselToNextPhysicsBubble(enterprise_d, std::move(parts));
      } else {
        KeepVessel(enterprise_d);
      }
      if (AbsoluteError(t - HistoryTime(sync_time, 2), half_a_step) < ε_δt) {
        ++expected_number_of_dirty_old_on_rails_vessels;
        expect_to_have_physics_bubble = false;
      }
      plugin_->AdvanceTime(t, planetarium_rotation);
      if (expect_to_have_physics_bubble) {
        plugin_->BubbleDisplacementCorrection(World::origin);
        plugin_->BubbleVelocityCorrection(SolarSystem::kSaturn);
      }
      if (AbsoluteError(t - HistoryTime(sync_time, 0), a_while) < ε_δt) {
        InsertVessel(enterprise_d,
                     &expected_number_of_new_off_rails_vessels,
                     t);
        --expected_number_of_new_off_rails_vessels;
      } else if (AbsoluteError(t - HistoryTime(sync_time, 1),
                               half_a_step) < ε_δt) {
        InsertVessel(enterprise_d_saucer,
                     &expected_number_of_new_off_rails_vessels,
                     t);
        --expected_number_of_new_off_rails_vessels;
      }
      expect_intrinsic_acceleration = expect_to_have_physics_bubble;
    }
    // Keep the vessels for the history-advancing step.
    // Keep the synchronizer.
    KeepVessel(constantinople);
    if (step <= 0) {
      KeepVessel(enterprise);
      parts.clear();
      parts.emplace_back(std::move(make_enterprise_whole_ship()));
      plugin_->AddVesselToNextPhysicsBubble(enterprise, std::move(parts));
    }
    KeepVessel(enterprise_d);
    if (step <= 1) {
      parts.clear();
      parts.emplace_back(std::move(make_enterprise_d_engineering_section()));
      parts.emplace_back(std::move(make_enterprise_d_saucer_section()));
      plugin_->AddVesselToNextPhysicsBubble(enterprise_d, std::move(parts));
    }
    expected_number_of_clean_old_vessels +=
        expected_number_of_new_off_rails_vessels +
        expected_number_of_dirty_old_on_rails_vessels;
    expected_number_of_new_off_rails_vessels = 0;
    expected_number_of_dirty_old_on_rails_vessels = 0;
    plugin_->AdvanceTime(HistoryTime(sync_time, step + 1) + δt,
                         planetarium_rotation);
    if (expect_to_have_physics_bubble) {
      plugin_->BubbleDisplacementCorrection(World::origin);
      plugin_->BubbleVelocityCorrection(SolarSystem::kSaturn);
    }
  }
}
#endif

TEST_F(PluginIntegrationTest, BodyCentredNonrotatingRenderingIntegration) {
  InsertAllSolarSystemBodies();
  plugin_->EndInitialization();
  GUID const satellite = "satellite";
  plugin_->InsertOrKeepVessel(satellite, SolarSystem::kEarth);
  plugin_->SetVesselStateOffset(satellite,
                                RelativeDegreesOfFreedom<AliceSun>(
                                    satellite_initial_displacement_,
                                    satellite_initial_velocity_));
  not_null<std::unique_ptr<RenderingTransforms>> const geocentric =
      plugin_->NewBodyCentredNonRotatingTransforms(SolarSystem::kEarth);
  // We'll check that our orbit is rendered as circular (actually, we only check
  // that it is rendered within a thin spherical shell around the Earth).
  Length perigee = std::numeric_limits<double>::infinity() * Metre;
  Length apogee = -std::numeric_limits<double>::infinity() * Metre;
  Permutation<AliceSun, World> const alice_sun_to_world =
      Permutation<AliceSun, World>(Permutation<AliceSun, World>::XZY);
  Time const δt_long = 10 * Minute;
#if !defined(_DEBUG)
  Time const δt_short = 0.02 * Second;
  Instant t = initial_time_ + δt_short;
  // Exercise #267 by having small time steps at the beginning of the trajectory
  // that are not synchronized with those of the Earth.
  for (; t < initial_time_ + δt_long; t += δt_short) {
    plugin_->AdvanceTime(t,
                         1 * Radian / Pow<2>(Minute) * Pow<2>(t - initial_time_));
    plugin_->InsertOrKeepVessel(satellite, SolarSystem::kEarth);
  }
#else
  Instant t = initial_time_ + δt_long;
  plugin_->AdvanceTime(t, 0 * Radian);  // This ensures the history is nonempty.
  plugin_->InsertOrKeepVessel(satellite, SolarSystem::kEarth);
  t += δt_long;
#endif
  for (; t < initial_time_ + 12 * Hour; t += δt_long) {
    plugin_->AdvanceTime(t,
                       1 * Radian / Pow<2>(Minute) * Pow<2>(t - initial_time_));
    plugin_->InsertOrKeepVessel(satellite, SolarSystem::kEarth);
    // We give the sun an arbitrary nonzero velocity in |World|.
    Position<World> const sun_world_position =
        World::origin + Velocity<World>(
            { 0.1 * AstronomicalUnit / Hour,
             -1.0 * AstronomicalUnit / Hour,
              0.0 * AstronomicalUnit / Hour}) * (t - initial_time_);
    RenderedTrajectory<World> const rendered_trajectory =
        plugin_->RenderedVesselTrajectory(satellite,
                                        geocentric.get(),
                                        sun_world_position);
    Position<World> const earth_world_position =
        sun_world_position + alice_sun_to_world(
            plugin_->CelestialFromParent(SolarSystem::kEarth).displacement());
    for (auto const segment : rendered_trajectory) {
      Length const l_min =
          std::min((segment.begin - earth_world_position).Norm(),
                   (segment.end - earth_world_position).Norm());
      Length const l_max =
          std::max((segment.begin - earth_world_position).Norm(),
                   (segment.end - earth_world_position).Norm());
      perigee = std::min(perigee, l_min);
      apogee = std::max(apogee, l_max);
    }
    // Check continuity.
    for (std::size_t i = 0; i + 1 < rendered_trajectory.size(); ++i) {
      EXPECT_THAT(rendered_trajectory[i].end,
                  Eq(rendered_trajectory[i + 1].begin));
    }
    EXPECT_THAT(Abs(apogee - perigee), Lt(3 * Metre));
  }
}

TEST_F(PluginIntegrationTest, BarycentricRotatingRenderingIntegration) {
  InsertAllSolarSystemBodies();
  plugin_->EndInitialization();
  GUID const satellite = "satellite";
  plugin_->InsertOrKeepVessel(satellite, SolarSystem::kEarth);
  // A vessel at the Lagrange point L₅.
  RelativeDegreesOfFreedom<AliceSun> const from_the_earth_to_the_moon =
      plugin_->CelestialFromParent(SolarSystem::kMoon);
  Displacement<AliceSun> const from_the_earth_to_l5 =
      from_the_earth_to_the_moon.displacement() / 2 -
          Normalize(from_the_earth_to_the_moon.velocity()) *
              from_the_earth_to_the_moon.displacement().Norm() * Sqrt(3) / 2;
  Velocity<AliceSun> const initial_velocity =
      Rotation<AliceSun, AliceSun>(
          π / 3 * Radian,
          Wedge(from_the_earth_to_the_moon.velocity(),
                from_the_earth_to_the_moon.displacement()))(
              from_the_earth_to_the_moon.velocity());
  plugin_->SetVesselStateOffset(satellite,
                                {from_the_earth_to_l5, initial_velocity});
  not_null<std::unique_ptr<RenderingTransforms>> const earth_moon_barycentric =
      plugin_->NewBarycentricRotatingTransforms(SolarSystem::kEarth,
                                                SolarSystem::kMoon);
  Permutation<AliceSun, World> const alice_sun_to_world =
      Permutation<AliceSun, World>(Permutation<AliceSun, World>::XZY);
  Time const δt_long = 1 * Hour;
#if defined(_DEBUG)
  Time const duration = 1 * Day;
  Instant t = initial_time_ + δt_long;
#else
  Time const δt_short = 0.02 * Second;
  Time const duration = 20 * Day;
  Instant t = initial_time_ + δt_short;
  // Exercise #267 by having small time steps at the beginning of the trajectory
  // that are not synchronized with those of the Earth and Moon.
  for (; t < initial_time_ + δt_long; t += δt_short) {
    plugin_->AdvanceTime(
        t,
        1 * Radian / Pow<2>(Minute) * Pow<2>(t - initial_time_));
    plugin_->InsertOrKeepVessel(satellite, SolarSystem::kEarth);
  }
#endif
  for (; t < initial_time_ + duration; t += δt_long) {
    plugin_->AdvanceTime(
        t,
        1 * Radian / Pow<2>(Minute) * Pow<2>(t - initial_time_));
    plugin_->InsertOrKeepVessel(satellite, SolarSystem::kEarth);
  }
  plugin_->AdvanceTime(t,
                       1 * Radian / Pow<2>(Minute) * Pow<2>(t - initial_time_));
  plugin_->InsertOrKeepVessel(satellite, SolarSystem::kEarth);
  // We give the sun an arbitrary nonzero velocity in |World|.
  Position<World> const sun_world_position =
      World::origin + Velocity<World>(
          { 0.1 * AstronomicalUnit / Hour,
           -1.0 * AstronomicalUnit / Hour,
            0.0 * AstronomicalUnit / Hour}) * (t - initial_time_);
  RenderedTrajectory<World> const rendered_trajectory =
      plugin_->RenderedVesselTrajectory(satellite,
                                      earth_moon_barycentric.get(),
                                      sun_world_position);
  Position<World> const earth_world_position =
      sun_world_position +
      alice_sun_to_world(
          plugin_->CelestialFromParent(SolarSystem::kEarth).displacement());
  Position<World> const moon_world_position =
      earth_world_position +
      alice_sun_to_world(
          plugin_->CelestialFromParent(SolarSystem::kMoon).displacement());
  Length const earth_moon =
      (moon_world_position - earth_world_position).Norm();
  for (auto const segment : rendered_trajectory) {
    Length const satellite_earth =
        (segment.begin - earth_world_position).Norm();
    Length const satellite_moon =
        (segment.begin - moon_world_position).Norm();
    EXPECT_THAT(RelativeError(earth_moon, satellite_earth), Lt(0.0907));
    EXPECT_THAT(RelativeError(earth_moon, satellite_moon), Lt(0.131));
    EXPECT_THAT(RelativeError(satellite_moon, satellite_earth), Lt(0.148));
  }
  // Check continuity.
  for (std::size_t i = 0; i + 1 < rendered_trajectory.size(); ++i) {
    EXPECT_THAT(rendered_trajectory[i].end,
                Eq(rendered_trajectory[i + 1].begin));
  }
#if !defined(_DEBUG)
  // Check that there are no spikes in the rendered trajectory, i.e., that three
  // consecutive points form a sufficiently flat triangle.  This tests issue
  // #256.
  for (std::size_t i = 0; i + 2 < rendered_trajectory.size(); ++i) {
    EXPECT_THAT(
        (rendered_trajectory[i].begin - rendered_trajectory[i + 1].end).Norm(),
        Gt(((rendered_trajectory[i].begin -
                 rendered_trajectory[i + 1].begin).Norm() +
            (rendered_trajectory[i].end -
                 rendered_trajectory[i + 1].end).Norm()) / 1.5)) << i;
  }
#endif
}

// Checks that we correctly predict a full circular orbit around a massive body
// with unit gravitational parameter at unit distance.  Since predictions are
// only computed on |AdvanceTime()|, we advance time by a small amount.
TEST_F(PluginIntegrationTest, Prediction) {
  GUID const satellite = "satellite";
  Index const celestial = 0;
  Plugin plugin(Instant(),
                0 * Radian);
  plugin.InsertSun(celestial, SIUnit<GravitationalParameter>());
  plugin.EndInitialization();
  EXPECT_TRUE(plugin.InsertOrKeepVessel(satellite, celestial));
  auto transforms = plugin.NewBodyCentredNonRotatingTransforms(celestial);
  plugin.SetVesselStateOffset(
      satellite,
      {Displacement<AliceSun>({1 * Metre, 0 * Metre, 0 * Metre}),
       Velocity<AliceSun>(
           {0 * Metre / Second, 1 * Metre / Second, 0 * Metre / Second})});
  plugin.set_predicted_vessel(satellite);
  plugin.set_prediction_length(2 * π * Second);
  plugin.set_prediction_length_tolerance(1 * Milli(Metre));
  plugin.set_prediction_speed_tolerance(1 * Milli(Metre) / Second);
  plugin.AdvanceTime(Instant(1e-10 * Second), 0 * Radian);
  RenderedTrajectory<World> rendered_prediction =
      plugin.RenderedPrediction(transforms.get(), World::origin);
  EXPECT_EQ(14, rendered_prediction.size());
  for (int i = 0; i < rendered_prediction.size(); ++i) {
    auto const& segment = rendered_prediction[i];
    EXPECT_THAT(
        AbsoluteError((segment.begin - World::origin).Norm(), 1 * Metre),
        Lt(0.5 * Milli(Metre)));
    EXPECT_THAT(AbsoluteError((segment.end - World::origin).Norm(), 1 * Metre),
                Lt(0.5 * Milli(Metre)));
    if (i >= 5) {
      EXPECT_THAT(
          AbsoluteError((segment.begin - World::origin).Norm(), 1 * Metre),
          Gt(0.1 * Milli(Metre)));
      EXPECT_THAT(
          AbsoluteError((segment.end - World::origin).Norm(), 1 * Metre),
          Gt(0.1 * Milli(Metre)));
    }
  }
  EXPECT_THAT(
      AbsoluteError(rendered_prediction.back().end - World::origin,
                    Displacement<World>({1 * Metre, 0 * Metre, 0 * Metre})),
      AllOf(Gt(2 * Milli(Metre)), Lt(3 * Milli(Metre))));
  plugin.clear_predicted_vessel();
}

}  // namespace ksp_plugin
}  // namespace principia
