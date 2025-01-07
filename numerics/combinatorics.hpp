#pragma once

#include <cstdint>

namespace principia {
namespace numerics {
namespace _combinatorics {
namespace internal {

// n choose k.
constexpr std::int64_t Binomial(std::int64_t n, std::int64_t k);

constexpr std::int64_t DoubleFactorial(std::int64_t n);

constexpr std::int64_t Factorial(std::int64_t n);

constexpr std::int64_t FallingFactorial(std::int64_t n, std::int64_t k);

constexpr std::int64_t KroneckerDelta(std::int64_t i, std::int64_t j);

}  // namespace internal

using internal::Binomial;
using internal::DoubleFactorial;
using internal::Factorial;
using internal::FallingFactorial;
using internal::KroneckerDelta;

}  // namespace _combinatorics
}  // namespace numerics
}  // namespace principia

#include "numerics/combinatorics_body.hpp"
