
#pragma once

#include <cstdint>

namespace principia {
namespace quantities {
namespace internal_dimensions {

// Dimensionality of physical quantities.  Note that we strongly type angles.
template<std::int64_t LengthExponent,
         std::int64_t MassExponent,
         std::int64_t TimeExponent,
         std::int64_t CurrentExponent,
         std::int64_t TemperatureExponent,
         std::int64_t AmountExponent,
         std::int64_t LuminousIntensityExponent,
         std::int64_t AngleExponent>
struct Dimensions;

// A double by any other name...
using NoDimensions = Dimensions<0, 0, 0, 0, 0, 0, 0, 0>;

// These structs have a |Type| member that is a |Dimensions| suitable for
// the result of the operation applied to argument(s) having the |Dimensions|
// given as template parameter(s).
template<int n, typename Dimensions>
struct DimensionsNthRootGenerator;
template<typename LDimensions, typename RDimensions>
struct DimensionsProductGenerator;
template<typename LDimensions, typename RDimensions>
struct DimensionsQuotientGenerator;

}  // namespace internal_dimensions

using internal_dimensions::Dimensions;
using internal_dimensions::DimensionsNthRootGenerator;
using internal_dimensions::DimensionsProductGenerator;
using internal_dimensions::DimensionsQuotientGenerator;
using internal_dimensions::NoDimensions;

}  // namespace quantities
}  // namespace principia

#include "quantities/dimensions_body.hpp"
