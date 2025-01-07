// .\Release\x64\benchmarks.exe --benchmark_repetitions=10 --benchmark_filter=Elliptic  // NOLINT(whitespace/line_length)

#include <random>
#include <vector>

#include "base/tags.hpp"
#include "benchmark/benchmark.h"
#include "numerics/elliptic_integrals.hpp"
#include "quantities/quantities.hpp"
#include "quantities/si.hpp"

namespace principia {
namespace numerics {

using namespace principia::base::_tags;
using namespace principia::numerics::_elliptic_integrals;
using namespace principia::quantities::_quantities;
using namespace principia::quantities::_si;

void BM_EllipticF(benchmark::State& state) {
  constexpr int size = 20;

  std::mt19937_64 random(42);
  std::uniform_real_distribution<> distribution_φ(0.0, π / 2);
  std::uniform_real_distribution<> distribution_mc(0.0, 1.0);
  std::vector<Angle> φs;
  std::vector<double> mcs;
  for (int i = 0; i < size; ++i) {
    φs.push_back(distribution_φ(random) * Radian);
    mcs.push_back(distribution_mc(random));
  }

  while (state.KeepRunningBatch(size * size)) {
    Angle f;
    for (Angle const φ : φs) {
      for (double const mc : mcs) {
        f += EllipticF(φ, mc);
      }
    }
    benchmark::DoNotOptimize(f);
  }
}

void BM_EllipticFEΠ(benchmark::State& state) {
  constexpr int size = 20;

  std::mt19937_64 random(42);
  std::uniform_real_distribution<> distribution_φ(0.0, π / 2);
  std::uniform_real_distribution<> distribution_n(0.0, 1.0);
  std::uniform_real_distribution<> distribution_mc(0.0, 1.0);
  std::vector<Angle> φs;
  std::vector<double> ns;
  std::vector<double> mcs;
  for (int i = 0; i < size; ++i) {
    φs.push_back(distribution_φ(random) * Radian);
    ns.push_back(distribution_n(random));
    mcs.push_back(distribution_mc(random));
  }

  while (state.KeepRunningBatch(size * size * size)) {
    Angle e{uninitialized};
    Angle f{uninitialized};
    Angle ᴨ{uninitialized};
    for (Angle const φ : φs) {
      for (double const n : ns) {
        for (double const mc : mcs) {
          EllipticFEΠ(φ, n, mc, f, e, ᴨ);
        }
      }
    }
    benchmark::DoNotOptimize(e);
    benchmark::DoNotOptimize(f);
    benchmark::DoNotOptimize(ᴨ);
  }
}

void BM_FukushimaEllipticBDJ(benchmark::State& state) {
  constexpr int size = 20;

  std::mt19937_64 random(42);
  std::uniform_real_distribution<> distribution_φ(0.0, π / 2);
  std::uniform_real_distribution<> distribution_n(0.0, 1.0);
  std::uniform_real_distribution<> distribution_mc(0.0, 1.0);
  std::vector<Angle> φs;
  std::vector<double> ns;
  std::vector<double> mcs;
  for (int i = 0; i < size; ++i) {
    φs.push_back(distribution_φ(random) * Radian);
    ns.push_back(distribution_n(random));
    mcs.push_back(distribution_mc(random));
  }

  while (state.KeepRunningBatch(size * size * size)) {
    Angle b{uninitialized};
    Angle d{uninitialized};
    Angle j{uninitialized};
    for (Angle const φ : φs) {
      for (double const n : ns) {
        for (double const mc : mcs) {
          FukushimaEllipticBDJ(φ, n, mc, b, d, j);
        }
      }
    }
    benchmark::DoNotOptimize(b);
    benchmark::DoNotOptimize(d);
    benchmark::DoNotOptimize(j);
  }
}

BENCHMARK(BM_EllipticF);
BENCHMARK(BM_EllipticFEΠ);
BENCHMARK(BM_FukushimaEllipticBDJ);

}  // namespace numerics
}  // namespace principia
