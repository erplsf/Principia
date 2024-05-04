#pragma once

#include "functions/accurate_table_generator.hpp"

#include <algorithm>
#include <future>
#include <limits>
#include <memory>
#include <thread>
#include <vector>

#include "base/for_all_of.hpp"
#include "base/not_null.hpp"
#include "base/thread_pool.hpp"
#include "glog/logging.h"
#include "numerics/fixed_arrays.hpp"
#include "numerics/lattices.hpp"
#include "numerics/matrix_computations.hpp"
#include "numerics/matrix_views.hpp"

namespace principia {
namespace functions {
namespace _accurate_table_generator {
namespace internal {

using namespace principia::base::_for_all_of;
using namespace principia::base::_not_null;
using namespace principia::base::_thread_pool;
using namespace principia::numerics::_fixed_arrays;
using namespace principia::numerics::_lattices;
using namespace principia::numerics::_matrix_computations;
using namespace principia::numerics::_matrix_views;

template<std::int64_t zeroes>
bool HasDesiredZeroes(cpp_bin_float_50 const& y) {
  std::int64_t y_exponent;
  auto const y_mantissa = frexp(y, &y_exponent);
  auto const y_mantissa_scaled =
      ldexp(y_mantissa, std::numeric_limits<double>::digits);
  auto const y_post_mantissa = y_mantissa_scaled - floor(y_mantissa_scaled);
  auto const y_candidate_zeroes = ldexp(y_post_mantissa, zeroes);
  return trunc(y_candidate_zeroes) == 0;
}

template<std::int64_t zeroes>
std::vector<cpp_rational> ExhaustiveMultisearch(
    std::vector<AccurateFunction> const& functions,
    std::vector<cpp_rational> const& starting_arguments) {
  ThreadPool<cpp_rational> search_pool(std::thread::hardware_concurrency());

  std::vector<std::future<cpp_rational>> futures;
  for (std::int64_t i = 0; i < starting_arguments.size(); ++i) {
    futures.push_back(search_pool.Add([i, &functions, &starting_arguments]() {
      return ExhaustiveSearch<zeroes>(functions, starting_arguments[i]);
    }));
  }

  std::vector<cpp_rational> results;
  for (auto& future : futures) {
    results.push_back(future.get());
  }
  return results;
}

template<std::int64_t zeroes>
cpp_rational ExhaustiveSearch(std::vector<AccurateFunction> const& functions,
                              cpp_rational const& starting_argument) {
  CHECK_LT(0, starting_argument);

  // We will look for candidates both above and below |starting_argument|.
  // Note that if |starting_argument| is a power of 2, the increments above
  // and below |starting_argument| are not the same.
  std::int64_t exponent;
  auto const starting_mantissa =
      frexp(static_cast<cpp_bin_float_50>(starting_argument), &exponent);
  cpp_rational const high_increment =
      exp2(exponent - std::numeric_limits<double>::digits);
  cpp_rational const low_increment =
      starting_mantissa == 0.5 ? high_increment / 2 : high_increment;

  cpp_rational high_x = starting_argument;
  cpp_rational low_x = starting_argument - low_increment;
  for (;;) {
    if (std::all_of(functions.begin(), functions.end(),
                    [&high_x](AccurateFunction const& f) {
                      return HasDesiredZeroes<zeroes>(f(high_x));
                    })) {
      return high_x;
    }
    high_x += high_increment;
    if (std::all_of(functions.begin(), functions.end(),
                    [&low_x](AccurateFunction const& f) {
                      return HasDesiredZeroes<zeroes>(f(low_x));
                    })) {
      return low_x;
    }
    low_x -= low_increment;
  }
}

template<std::int64_t zeroes>
absl::StatusOr<cpp_rational> SimultaneousBadCaseSearch(
  std::array<AccurateFunction, 2> const& functions,
  std::array<AccuratePolynomial<cpp_rational, 2>, 2> const& polynomials,
  cpp_rational const& near_argument,
  std::int64_t const M,
  std::int64_t const N,
  std::int64_t const T) {

  std::array<AccurateFunction, 2> F;
  std::array<std::optional<AccuratePolynomial<cpp_rational, 2>>, 2> P;

  for (std::int64_t i = 0; i < functions.size(); ++i) {
    F[i] = [&functions, i, N, &near_argument](cpp_rational const& t) {
      // Here |t| <= |T|.
      return N * functions[i](near_argument + t / N);
    };
  }
  AccuratePolynomial<cpp_rational, 1> const shift_and_rescale(
      {near_argument, cpp_rational(1, N)});
  for (std::int64_t i = 0; i < polynomials.size(); ++i) {
    P[i] = N * Compose(polynomials[i], shift_and_rescale);
  }

  cpp_rational const T_increment = cpp_rational(T, 100);///???
  cpp_bin_float_50 ε = 0;
  for (std::int64_t i = 0; i < 2; ++i) {
    for (cpp_rational t = -T; t <= T; t += T_increment) {
      ε = std::max(ε, abs(F[i](t) - static_cast<cpp_bin_float_50>((*P[i])(t))));
    }
  }
LOG(ERROR)<<"ε: "<<ε;

  auto const Mʹ = static_cast<std::int64_t>(floor(M / (2 + 2 * M * ε)));
  auto const C = 3 * Mʹ;
  if (C == 0) {
    return absl::FailedPreconditionError("Error too large");
  }
LOG(ERROR)<<"C:"<<C;
  std::array<std::optional<AccuratePolynomial<cpp_int, 2>>, 2> P̃;
  AccuratePolynomial<cpp_rational, 1> const Tτ({0, T});
  for (std::int64_t i = 0; i < 2; ++i) {
LOG(ERROR)<<"P: "<<*P[i];
    auto const composition_coefficients = Compose(C * *P[i], Tτ).coefficients();
    AccuratePolynomial<cpp_int, 2>::Coefficients P̃_coefficients;
    for_all_of(composition_coefficients, P̃_coefficients)
        .loop([](auto const& composition_coefficient, auto& P̃_coefficient) {
          P̃_coefficient = static_cast<cpp_int>(
              round(static_cast<cpp_bin_float_50>(composition_coefficient)));
        });
    P̃[i] = AccuratePolynomial<cpp_int, 2>(P̃_coefficients);
    LOG(ERROR)<<"i: "<<i<<" P̃: "<<*P̃[i];
  }

  auto const& P̃₀_coefficients = P̃[0]->coefficients();
  auto const& P̃₁_coefficients = P̃[1]->coefficients();
  using Lattice = FixedMatrix<cpp_int, 5, 4>;

  Lattice const L(
      {C,     0, std::get<0>(P̃₀_coefficients), std::get<0>(P̃₁_coefficients),
       0, C * T, std::get<1>(P̃₀_coefficients), std::get<1>(P̃₁_coefficients),
       0,     0, std::get<2>(P̃₀_coefficients), std::get<2>(P̃₁_coefficients),
       0,     0,                            3,                            0,
       0,     0,                            0,                            3});
LOG(ERROR)<<"L:"<<L;

  Lattice const V = LenstraLenstraLovász(L);
LOG(ERROR)<<"V:"<<V;

  std::array<std::unique_ptr<ColumnView<Lattice const>>, V.columns()> v;
  for (std::int64_t i = 0; i < v.size(); ++i) {
    v[i] = std::make_unique<ColumnView<Lattice const>>(
        ColumnView{.matrix = V,
                   .first_row = 0,
                   .last_row = V.rows() - 1,
                   .column = static_cast<int>(i)});///??
  }

  auto norm1 = [](ColumnView<Lattice const> const& v) {
    cpp_rational norm1 = 0;
    for (std::int64_t i = 0; i < v.size(); ++i) {
      norm1 += abs(v[i]);
    }
    return norm1;
  };

  std::sort(v.begin(),
            v.end(),
            [&norm1](std::unique_ptr<ColumnView<Lattice const>> const& left,
                     std::unique_ptr<ColumnView<Lattice const>> const& right) {
              return left->Norm²() < right->Norm²();
            });

  static constexpr std::int64_t dimension = 3;
  for (std::int64_t i = 0; i < dimension; ++i) {
    auto const& v_i = *v[i];
LOG(ERROR)<<"i: "<<i<<" v_i: "<<v_i;
    if (norm1(v_i) >= C) {
      return absl::NotFoundError("Vectors too big");
    }
  }

  std::array<cpp_int, dimension> Q_multipliers;
  for (std::int64_t i = 0; i < dimension; ++i) {
    auto const& v_i1 = *v[(i + 1) % dimension];
    auto const& v_i2 = *v[(i + 2) % dimension];
    Q_multipliers[i] = v_i1[3] * v_i2[4] - v_i1[4] * v_i2[3];
LOG(ERROR)<<"Qmu: "<<Q_multipliers[i];
  }

  FixedVector<cpp_int, 2> Q_coefficients{};
  for (std::int64_t i = 0; i < dimension; ++i) {
    auto const& v_i = *v[i];
    for (std::int64_t j = 0; j < Q_coefficients.size(); ++j) {
      Q_coefficients[j] += Q_multipliers[i] * v_i[j];
    }
LOG(ERROR)<<"Qcoeffs: "<<Q_coefficients;
  }

  if (Q_coefficients[1] == 0) {
      return absl::NotFoundError("No integer zeroes");
  }

  AccuratePolynomial<cpp_rational, 1> const Q({Q_coefficients[0],
                                               Q_coefficients[1]});
  LOG(ERROR)<<"Q: "<<Q;
  AccuratePolynomial<cpp_rational, 1> const q =
      Compose(Q, AccuratePolynomial<cpp_rational, 1>({0, cpp_rational(1, T)}));
  LOG(ERROR)<<"q: "<<q;

  cpp_rational const t₀ =
      -std::get<0>(q.coefficients()) / std::get<1>(q.coefficients());
LOG(ERROR)<<"t₀: "<<t₀;
  if (abs(t₀) > T) {
    return absl::NotFoundError("Out of bounds");
  } else if (denominator(t₀) != 1) {
    return absl::NotFoundError("Noninteger root");
  }

  for (auto const& Fi : F) {
    auto const Fi_t₀ = static_cast<cpp_bin_float_50>(Fi(t₀));
    if (abs(Fi_t₀ - round(Fi_t₀)) >= 1 / cpp_bin_float_50(M)) {
        LOG(ERROR) << Fi_t₀ - round(Fi_t₀);
        return absl::NotFoundError("Not enough zeroes");
      }
  }

  return t₀ / N + near_argument;
}

}  // namespace internal
}  // namespace _accurate_table_generator
}  // namespace functions
}  // namespace principia
