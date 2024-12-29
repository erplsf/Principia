#include <emmintrin.h>

#include "nanobenchmarks/function_registry.hpp"
#include "numerics/cbrt.hpp"

namespace principia {
namespace nanobenchmarks {
namespace _examples {

using namespace principia::numerics::_cbrt;

BENCHMARKED_FUNCTION(twice) {
  return 2 * x;
}

BENCHMARKED_FUNCTION(thrice) {
  return 3 * x;
}

BENCHMARKED_FUNCTION(inc) {
  return x + 1;
}

BENCHMARKED_FUNCTION(add_4_times) {
  return x * x * x * x * x;
}

BENCHMARKED_FUNCTION(add_16_times) {
  return x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x + x;
}

BENCHMARKED_FUNCTION(square_root) {
  __m128d x_0 = _mm_set_sd(x);
  return _mm_cvtsd_f64(_mm_sqrt_sd(x_0, x_0));
}

BENCHMARKED_FUNCTION(sqrt_sqrt) {
  __m128d x_0 = _mm_set_sd(x);
  x_0 = _mm_sqrt_sd(x_0, x_0);
  return _mm_cvtsd_f64(_mm_sqrt_sd(x_0, x_0));
}

BENCHMARKED_FUNCTION(square_root_division) {
  __m128d x_0 = _mm_set_sd(x);
  return _mm_cvtsd_f64(_mm_div_sd(x_0, _mm_sqrt_sd(x_0, x_0)));
}
BENCHMARK_FUNCTION(Cbrt);

using namespace principia::numerics::_cbrt::internal;

BENCHMARK_FUNCTION(method_3²ᴄZ5¹::Cbrt<Rounding::Faithful>);
BENCHMARK_FUNCTION(method_3²ᴄZ5¹::Cbrt<Rounding::Correct>);
BENCHMARK_FUNCTION(method_5²Z4¹FMA::Cbrt<Rounding::Faithful>);
BENCHMARK_FUNCTION(method_5²Z4¹FMA::Cbrt<Rounding::Correct>);

}  // namespace _examples
}  // namespace nanobenchmarks
}  // namespace principia
