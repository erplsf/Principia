#pragma once

#include <list>

#include "gmock/gmock.h"
#include "ksp_plugin/vessel.hpp"  // 🧙 For Vessel.

namespace principia {
namespace ksp_plugin {
namespace _vessel {
namespace internal {

class MockVessel : public Vessel {
 public:
  MockVessel() = default;

  MOCK_METHOD(not_null<MasslessBody const*>, body, (), (const, override));

  MOCK_METHOD(not_null<Celestial const*>, parent, (), (const, override));
  MOCK_METHOD(void,
              set_parent,
              (not_null<Celestial const*> parent),
              (override));

  MOCK_METHOD(DiscreteTrajectorySegmentIterator<Barycentric>,
              psychohistory,
              (),
              (const, override));
  MOCK_METHOD(DiscreteTrajectorySegmentIterator<Barycentric>,
              prediction,
              (),
              (const, override));

  MOCK_METHOD(FlightPlan&, flight_plan, (), (const, override));
  MOCK_METHOD(bool, has_flight_plan, (), (const, override));

  MOCK_METHOD(void,
              CreateFlightPlan,
              (Instant const& final_time,
               Mass const& initial_mass,
               Ephemeris<Barycentric>::AdaptiveStepParameters const&
                   flight_plan_adaptive_step_parameters,
               Ephemeris<Barycentric>::GeneralizedAdaptiveStepParameters const&
                   flight_plan_generalized_adaptive_step_parameters),
              (override));

  MOCK_METHOD(void, DeleteFlightPlan, (), (override));

  MOCK_METHOD(void,
              WriteToMessage,
              (not_null<serialization::Vessel*> message,
               PileUp::SerializationIndexForPileUp const&
                   serialization_index_for_pile_up),
              (const, override));
};

}  // namespace internal

using internal::MockVessel;

}  // namespace _vessel
}  // namespace ksp_plugin
}  // namespace principia
