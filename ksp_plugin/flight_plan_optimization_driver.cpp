#include "ksp_plugin/flight_plan_optimization_driver.hpp"

#include <functional>

#include "absl/status/status.h"

namespace principia {
namespace ksp_plugin {
namespace _flight_plan_optimization_driver {
namespace internal {

using std::placeholders::_1;

FlightPlanOptimizationDriver::FlightPlanOptimizationDriver(
    FlightPlan const& flight_plan,
    FlightPlanOptimizer::MetricFactory metric_factory)
    : flight_plan_under_optimization_(flight_plan),
      flight_plan_optimizer_(
          &flight_plan_under_optimization_,
          std::move(metric_factory),
          std::bind(&FlightPlanOptimizationDriver::UpdateLastFlightPlan,
                    this,
                    _1)),
      last_flight_plan_(
          make_not_null_shared<FlightPlan>(flight_plan_under_optimization_)) {}

FlightPlanOptimizationDriver::~FlightPlanOptimizationDriver() {
  // Ensure that we do not have a thread still running with references to the
  // members of this class when those are destroyed.
  Interrupt();
}

std::shared_ptr<FlightPlan>
FlightPlanOptimizationDriver::last_flight_plan() const {
  absl::ReaderMutexLock l(&lock_);
  return last_flight_plan_;
}

bool FlightPlanOptimizationDriver::done() const {
  absl::ReaderMutexLock l(&lock_);
  return optimizer_idle_;
}

void FlightPlanOptimizationDriver::Interrupt() {
  optimizer_ = jthread();
  // We are single-threaded here, no need to lock.
  optimizer_idle_ = true;
}

void FlightPlanOptimizationDriver::RequestOptimization(
    Parameters const& parameters) {
  // Only process this request if there is no analysis in progress.
  absl::MutexLock l(&lock_);
  if (optimizer_idle_) {
    optimizer_idle_ = false;
    optimizer_ = MakeStoppableThread([this, parameters]() {
      const absl::Status optimization_status = flight_plan_optimizer_.Optimize(
          parameters.index, parameters.Δv_tolerance);

      absl::MutexLock l(&lock_);
      if (optimization_status.ok()) {
        last_flight_plan_ =
            make_not_null_shared<FlightPlan>(flight_plan_under_optimization_);
      }
      optimizer_idle_ = true;
    });
  }
}

void FlightPlanOptimizationDriver::Wait() const {
  absl::ReaderMutexLock l(&lock_);
  lock_.Await(absl::Condition(&optimizer_idle_));
}

void FlightPlanOptimizationDriver::UpdateLastFlightPlan(
    FlightPlan const& flight_plan) {
  absl::MutexLock l(&lock_);
  last_flight_plan_ = make_not_null_shared<FlightPlan>(flight_plan);
}

}  // namespace internal
}  // namespace _flight_plan_optimization_driver
}  // namespace ksp_plugin
}  // namespace principia
