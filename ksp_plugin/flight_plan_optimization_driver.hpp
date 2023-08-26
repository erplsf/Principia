#pragma once

#include <memory>

#include "absl/synchronization/mutex.h"
#include "base/jthread.hpp"
#include "base/not_null.hpp"
#include "ksp_plugin/celestial.hpp"
#include "ksp_plugin/flight_plan.hpp"
#include "ksp_plugin/flight_plan_optimizer.hpp"
#include "quantities/named_quantities.hpp"
#include "quantities/quantities.hpp"

namespace principia {
namespace ksp_plugin {
namespace _flight_plan_optimization_driver {
namespace internal {

using namespace principia::base::_jthread;
using namespace principia::base::_not_null;
using namespace principia::ksp_plugin::_celestial;
using namespace principia::ksp_plugin::_flight_plan;
using namespace principia::ksp_plugin::_flight_plan_optimizer;
using namespace principia::quantities::_named_quantities;
using namespace principia::quantities::_quantities;

class FlightPlanOptimizationDriver {
 public:
  struct Parameters {
    int index;
    not_null<Celestial const*> celestial;
    Length target_distance;
    Speed Δv_tolerance;
  };

  explicit FlightPlanOptimizationDriver(FlightPlan const& flight_plan);

  virtual ~FlightPlanOptimizationDriver();

  // Returns the last flight plan evaluated by the optimizer.
  std::shared_ptr<FlightPlan> last_flight_plan() const;

  // Cancels any optimization in progress.
  void Interrupt();

  // Starts an optimization with the given parameters.  Has no effect if an
  // optimization is already happening.
  void RequestOptimization(Parameters const& parameters);

  // Waits for the current optimization (if any) to complete.
  void Wait() const;

 private:
  void UpdateLastFlightPlan(FlightPlan const& flight_plan);

  // The flight plan being optimized, asynchronously modified by the optimizer.
  FlightPlan flight_plan_under_optimization_;
  FlightPlanOptimizer flight_plan_optimizer_;

  mutable absl::Mutex lock_;
  jthread optimizer_;
  bool optimizer_idle_ GUARDED_BY(lock_) = true;

  // The last flight plan evaluated by the optimizer.
  not_null<std::shared_ptr<FlightPlan>> last_flight_plan_ GUARDED_BY(lock_);
};

}  // namespace internal

using internal::FlightPlanOptimizationDriver;

}  // namespace _flight_plan_optimization_driver
}  // namespace ksp_plugin
}  // namespace principia
