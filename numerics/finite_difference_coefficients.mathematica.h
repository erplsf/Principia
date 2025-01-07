#pragma once

#include <array>
#include <tuple>

#include "numerics/fixed_arrays.hpp"

namespace principia {
namespace numerics {
namespace _finite_difference_coefficients {
namespace internal {

using namespace principia::numerics::_fixed_arrays;

constexpr auto Numerators = std::make_tuple(
    FixedMatrix<double, 1, 1>{{
        0,
    }},
    FixedMatrix<double, 2, 2>{{
        -1, 1,
        -1, 1,
    }},
    FixedMatrix<double, 3, 3>{{
        -3,  4, -1,
        -1,  0,  1,
         1, -4,  3,
    }},
    FixedMatrix<double, 4, 4>{{
        -11, 18,  -9,  2,
         -2, -3,   6, -1,
          1, -6,   3,  2,
         -2,  9, -18, 11,
    }},
    FixedMatrix<double, 5, 5>{{
        -25,  48, -36,  16, -3,
         -3, -10,  18,  -6,  1,
          1,  -8,   0,   8, -1,
         -1,   6, -18,  10,  3,
          3, -16,  36, -48, 25,
    }},
    FixedMatrix<double, 6, 6>{{
        -137, 300, -300,  200,  -75,  12,
         -12, -65,  120,  -60,   20,  -3,
           3, -30,  -20,   60,  -15,   2,
          -2,  15,  -60,   20,   30,  -3,
           3, -20,   60, -120,   65,  12,
         -12,  75, -200,  300, -300, 137,
    }},
    FixedMatrix<double, 7, 7>{{
        -147, 360, -450,  400, -225,   72, -10,
         -10, -77,  150, -100,   50,  -15,   2,
           2, -24,  -35,   80,  -30,    8,  -1,
          -1,   9,  -45,    0,   45,   -9,   1,
           1,  -8,   30,  -80,   35,   24,  -2,
          -2,  15,  -50,  100, -150,   77,  10,
          10, -72,  225, -400,  450, -360, 147,
    }},
    FixedMatrix<double, 8, 8>{{
        -1089, 2940, -4410,  4900, -3675,  1764,  -490,   60,
          -60, -609,  1260, -1050,   700,  -315,    84,  -10,
           10, -140,  -329,   700,  -350,   140,   -35,    4,
           -4,   42,  -252,  -105,   420,  -126,    28,   -3,
            3,  -28,   126,  -420,   105,   252,   -42,    4,
           -4,   35,  -140,   350,  -700,   329,   140,  -10,
           10,  -84,   315,  -700,  1050, -1260,   609,   60,
          -60,  490, -1764,  3675, -4900,  4410, -2940, 1089,
    }},
    FixedMatrix<double, 9, 9>{{
        -2283,  6720, -11760, 15680, -14700,   9408, -3920,   960, -105,
         -105, -1338,   2940, -2940,   2450,  -1470,   588,  -140,   15,
           15,  -240,   -798,  1680,  -1050,    560,  -210,    48,   -5,
           -5,    60,   -420,  -378,   1050,   -420,   140,   -30,    3,
            3,   -32,    168,  -672,      0,    672,  -168,    32,   -3,
           -3,    30,   -140,   420,  -1050,    378,   420,   -60,    5,
            5,   -48,    210,  -560,   1050,  -1680,   798,   240,  -15,
          -15,   140,   -588,  1470,  -2450,   2940, -2940,  1338,  105,
          105,  -960,   3920, -9408,  14700, -15680, 11760, -6720, 2283,
    }});

constexpr std::array<double, 9> Denominators{
      1,
      1,
      2,
      6,
     12,
     60,
     60,
    420,
    840,
};

}  // namespace internal

using internal::Denominators;
using internal::Numerators;

}  // namespace _finite_difference_coefficients
}  // namespace numerics
}  // namespace principia
