// .\Release\x64\benchmarks.exe --benchmark_repetitions=10 --benchmark_filter=Jacobi  // NOLINT(whitespace/line_length)

#include <random>
#include <vector>

#include "benchmark/benchmark.h"
#include "numerics/elliptic_functions.hpp"
#include "quantities/quantities.hpp"
#include "quantities/si.hpp"

namespace principia {
namespace numerics {

using namespace principia::numerics::_elliptic_functions;
using namespace principia::quantities::_quantities;
using namespace principia::quantities::_si;

void BM_JacobiAmplitude(benchmark::State& state) {
  constexpr int size = 100;

  std::mt19937_64 random(42);
  std::uniform_real_distribution<> distribution_u(-10.0, 10.0);
  std::uniform_real_distribution<> distribution_mc(0.0, 1.0);
  std::vector<Angle> us;
  std::vector<double> mcs;
  for (int i = 0; i < size; ++i) {
    us.push_back(distribution_u(random) * Radian);
    mcs.push_back(distribution_mc(random));
  }

  while (state.KeepRunningBatch(size * size)) {
    Angle a;
    for (Angle const u : us) {
      for (double const mc : mcs) {
        a = JacobiAmplitude(u, mc);
      }
    }
    benchmark::DoNotOptimize(a);
  }
}

void BM_JacobiSNCNDN(benchmark::State& state) {
  constexpr int size = 100;

  std::mt19937_64 random(42);
  std::uniform_real_distribution<> distribution_u(-10.0, 10.0);
  std::uniform_real_distribution<> distribution_mc(0.0, 1.0);
  std::vector<Angle> us;
  std::vector<double> mcs;
  for (int i = 0; i < size; ++i) {
    us.push_back(distribution_u(random) * Radian);
    mcs.push_back(distribution_mc(random));
  }

  while (state.KeepRunningBatch(size * size)) {
    double s;
    double c;
    double d;
    for (Angle const u : us) {
      for (double const mc : mcs) {
        JacobiSNCNDN(u, mc, s, c, d);
      }
    }
    benchmark::DoNotOptimize(s);
    benchmark::DoNotOptimize(c);
    benchmark::DoNotOptimize(d);
  }
}

BENCHMARK(BM_JacobiAmplitude);
BENCHMARK(BM_JacobiSNCNDN);

}  // namespace numerics
}  // namespace principia
