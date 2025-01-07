#pragma once

#include "mathematica/local_error_analysis.hpp"

#include <filesystem>
#include <memory>
#include <utility>
#include <vector>

#include "astronomy/solar_system_fingerprints.hpp"
#include "astronomy/stabilize_ksp.hpp"
#include "base/file.hpp"
#include "mathematica/mathematica.hpp"
#include "physics/degrees_of_freedom.hpp"
#include "physics/massive_body.hpp"

namespace principia {
namespace mathematica {
namespace _local_error_analysis {
namespace internal {

using namespace principia::astronomy::_solar_system_fingerprints;
using namespace principia::astronomy::_stabilize_ksp;
using namespace principia::base::_file;
using namespace principia::mathematica::_mathematica;
using namespace principia::physics::_degrees_of_freedom;
using namespace principia::physics::_massive_body;

template<typename Frame>
LocalErrorAnalyser<Frame>::LocalErrorAnalyser(
    not_null<std::unique_ptr<SolarSystem<Frame>>> solar_system,
    FixedStepSizeIntegrator<typename
        Ephemeris<Frame>::NewtonianMotionEquation> const& integrator,
    Time const& step)
    : solar_system_(std::move(solar_system)),
      integrator_(integrator),
      step_(step) {
  if (solar_system_->Fingerprint() == KSPStockSystemFingerprints[KSP191]) {
    LOG(INFO) << "All hail retrobop!";
    StabilizeKSP(*solar_system_);
    CHECK_EQ(solar_system_->Fingerprint(),
             KSPStabilizedSystemFingerprints[KSP191]);
  }
}

template<typename Frame>
void LocalErrorAnalyser<Frame>::WriteLocalErrors(
    std::filesystem::path const& path,
    FixedStepSizeIntegrator<typename
        Ephemeris<Frame>::NewtonianMotionEquation> const& fine_integrator,
    Time const& fine_step,
    Time const& granularity,
    Time const& duration) const {
  auto const reference_ephemeris = solar_system_->MakeEphemeris(
      /*accuracy_parameters=*/{fitting_tolerance_,
                               /*geopotential_tolerance=*/0x1p-24},
      typename Ephemeris<Frame>::FixedStepParameters(integrator_, step_));
  CHECK_OK(reference_ephemeris->Prolong(solar_system_->epoch()));
  std::vector<std::vector<Length>> errors;
  for (Instant t0 = solar_system_->epoch(),
               t = t0 + granularity;
       t < solar_system_->epoch() + duration;
       t0 = t, t += granularity) {
    std::unique_ptr<Ephemeris<Frame>> refined_ephemeris =
        ForkEphemeris(*reference_ephemeris, t0, fine_integrator, fine_step);
    CHECK_OK(reference_ephemeris->Prolong(t));
    CHECK_OK(refined_ephemeris->Prolong(t));
    LOG_EVERY_N(INFO, 10) << "Prolonged to "
                          << (t - solar_system_->epoch()) / Day << " days.";

    errors.emplace_back();
    for (auto const& body_name : solar_system_->names()) {
      int const body_index = solar_system_->index(body_name);
      errors.back().push_back(
          (reference_ephemeris
               ->trajectory(reference_ephemeris->bodies()[body_index])
               ->EvaluatePosition(t) -
           refined_ephemeris
               ->trajectory(refined_ephemeris->bodies()[body_index])
               ->EvaluatePosition(t)).Norm());
    }
  }
  OFStream file(path);
  file << Set("bodyNames", solar_system_->names());
  file << Set("errors", errors, ExpressIn(Metre));
}

template<typename Frame>
not_null<std::unique_ptr<Ephemeris<Frame>>>
LocalErrorAnalyser<Frame>::ForkEphemeris(
    Ephemeris<Frame> const& original,
    Instant const& t,
    FixedStepSizeIntegrator<typename
        Ephemeris<Frame>::NewtonianMotionEquation> const& integrator,
    Time const& step) const {
  std::vector<DegreesOfFreedom<Frame>> degrees_of_freedom;
  for (not_null<MassiveBody const*> const body : original.bodies()) {
    degrees_of_freedom.emplace_back(
        original.trajectory(body)->EvaluateDegreesOfFreedom(t));
  }
  return make_not_null_unique<Ephemeris<Frame>>(
      solar_system_->MakeAllMassiveBodies(),
      degrees_of_freedom,
      t,
      typename Ephemeris<Frame>::AccuracyParameters(
          fitting_tolerance_, /*geopotential_tolerance=*/0x1p-24),
      typename Ephemeris<Frame>::FixedStepParameters(integrator, step));
}

}  // namespace internal
}  // namespace _local_error_analysis
}  // namespace mathematica
}  // namespace principia
