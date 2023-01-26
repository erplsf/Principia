#pragma once

#include <array>

namespace principia {
namespace testing_utilities {

// See https://www.sfu.ca/~ssurjano/branin.html.
double Branin(double x₁, double x₂);
std::array<double, 2> 𝛁Branin(double x₁, double x₂);

// See https://www.sfu.ca/~ssurjano/goldpr.html.
double GoldsteinPrice(double x₁, double x₂);
std::array<double, 2> 𝛁GoldsteinPrice(double x₁, double x₂);

// See https://www.sfu.ca/~ssurjano/hart3.html.
double Hartmann3(double x₁, double x₂, double x₃);
std::array<double, 3> 𝛁Hartmann3(double x₁, double x₂, double x₃);

}  // namespace testing_utilities
}  // namespace principia