#pragma once

#include <algorithm>
#include <type_traits>

#include "geometry/instant.hpp"
#include "geometry/interval.hpp"
#include "numerics/poisson_series.hpp"
#include "quantities/named_quantities.hpp"
#include "quantities/quantities.hpp"

namespace principia {
namespace numerics {
namespace _frequency_analysis {
namespace internal {

using namespace principia::geometry::_instant;
using namespace principia::geometry::_interval;
using namespace principia::numerics::_poisson_series;
using namespace principia::quantities::_named_quantities;
using namespace principia::quantities::_quantities;

// Computes the precise mode of a quasi-periodic `function`, assuming that the
// mode is over the interval `fft_mode` (so named because it has presumably been
// obtained using FFT).  The `Function` must have a member `FourierTransform`
// that returns its spectrum.  See [Cha95].
template<typename Function,
         int aperiodic_wdegree, int periodic_wdegree>
AngularFrequency PreciseMode(
    Interval<AngularFrequency> const& fft_mode,
    Function const& function,
    PoissonSeries<double,
                  aperiodic_wdegree, periodic_wdegree> const& weight);

// In the projection functions the `Function` must have an `InnerProduct` with
// `PoissonSeries` or `PiecewisePoissonSeries`.

// Computes the Кудрявцев projection of `function` on a basis with angular
// frequency ω and maximum degrees `aperiodic_ldegree` and `periodic_ldegree`.
// See [Kud07].
template<int aperiodic_degree, int periodic_degree,
         typename Function,
         int aperiodic_wdegree, int periodic_wdegree>
PoissonSeries<std::invoke_result_t<Function, Instant>,
              aperiodic_degree, periodic_degree>
Projection(Function const& function,
           AngularFrequency const& ω,
           PoissonSeries<double,
                         aperiodic_wdegree, periodic_wdegree> const& weight,
           Instant const& t_min,
           Instant const& t_max);

// AngularFrequencyCalculator is a templated functor that implements the
// extraction of the most relevant frequency out of a (mostly periodic)
// residual.  Its declaration must look like:
//
// class AngularFrequencyCalculator {
//  public:
//   ...
//   template<typename Residual>
//   std::optional<AngularFrequency>
//   operator()(Residual const& residual) const;
//   ...
//};
//
// Where Residual is a functor that takes an Instant and returns an element of a
// vector space.  The first call to the calculator is with the `function` passed
// to `IncrementalProjection`.
// If the calculator cannot find a suitable frequency, or if it wants to stop
// the algorithm, it does so by returning std::nullopt.
template<int aperiodic_degree, int periodic_degree,
         typename Function,
         typename AngularFrequencyCalculator,
         int aperiodic_wdegree, int periodic_wdegree>
PoissonSeries<std::invoke_result_t<Function, Instant>,
              aperiodic_degree, periodic_degree>
IncrementalProjection(
    Function const& function,
    AngularFrequencyCalculator const& calculator,
    PoissonSeries<double,
                  aperiodic_wdegree, periodic_wdegree> const& weight,
    Instant const& t_min,
    Instant const& t_max);

}  // namespace internal

using internal::IncrementalProjection;
using internal::PreciseMode;
using internal::Projection;

}  // namespace _frequency_analysis
}  // namespace numerics
}  // namespace principia

#include "numerics/frequency_analysis_body.hpp"
