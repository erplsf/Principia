#pragma once

#include <cstdint>

namespace principia {
namespace base {
namespace _bits {
namespace internal {

// Floor log2 of n, or 0 for n = 0.  8 ↦ 3, 7 ↦ 2.
constexpr std::int64_t FloorLog2(std::int64_t n);

// Greatest power of 2 less than or equal to n.  8 ↦ 8, 7 ↦ 4.
constexpr std::int64_t PowerOf2Le(std::int64_t n);

// Computes bitreversed(bitreversed(n) + 1) assuming that n is represented on
// the given number of bits.  For 4 bits:
//   0 ↦ 8 ↦ 4 ↦ C ↦ 2 ↦ A ↦ 6 ↦ E ↦ 1 ↦ 9 ↦ 5 ↦ D ↦ 3 ↦ B ↦ 7 ↦ F
constexpr std::int64_t BitReversedIncrement(std::int64_t n, std::int64_t bits);

}  // namespace internal

using internal::BitReversedIncrement;
using internal::FloorLog2;
using internal::PowerOf2Le;

}  // namespace _bits
}  // namespace base
}  // namespace principia

#include "base/bits_body.hpp"
