#pragma once

#include <cfloat>
#include <cstdint>

#include <complex>
#include <string>

#include "geometry/complexification.hpp"
#include "geometry/grassmann.hpp"
#include "geometry/point.hpp"
#include "geometry/quaternion.hpp"
#include "geometry/r3_element.hpp"
#include "geometry/r3x3_matrix.hpp"
#include "geometry/rotation.hpp"
#include "gmock/gmock.h"
#include "numerics/double_precision.hpp"
#include "numerics/fixed_arrays.hpp"
#include "numerics/unbounded_arrays.hpp"
#include "quantities/quantities.hpp"

namespace principia {
namespace testing_utilities {
namespace _almost_equals {
namespace internal {

using namespace principia::geometry::_complexification;
using namespace principia::geometry::_grassmann;
using namespace principia::geometry::_point;
using namespace principia::geometry::_quaternion;
using namespace principia::geometry::_r3_element;
using namespace principia::geometry::_r3x3_matrix;
using namespace principia::geometry::_rotation;
using namespace principia::numerics::_double_precision;
using namespace principia::numerics::_fixed_arrays;
using namespace principia::numerics::_unbounded_arrays;
using namespace principia::quantities::_quantities;

template<typename T>
class AlmostEqualsMatcher;

// The 2-argument version of `AlmostEquals()` should always be preferred as it
// guarantees that the error bound is tight.
template<typename T>
testing::PolymorphicMatcher<AlmostEqualsMatcher<T>> AlmostEquals(
    T const& expected,
    std::int64_t max_ulps);

// The 3-argument version of `AlmostEquals()` is exclusively for use when a
// given assertion may have different errors, e.g., because it's in a loop.  It
// doesn't guarantee that the error bound is tight.  For vectors, it applies
// only to the component with the largest error.
template<typename T>
testing::PolymorphicMatcher<AlmostEqualsMatcher<T>> AlmostEquals(
    T const& expected,
    std::int64_t min_ulps,
    std::int64_t max_ulps);

template<typename T>
class AlmostEqualsMatcher final {
 public:
  explicit AlmostEqualsMatcher(T const& expected,
                               std::int64_t min_ulps,
                               std::int64_t max_ulps);

  template<typename Dimensions>
  bool MatchAndExplain(Quantity<Dimensions> const& actual,
                       testing::MatchResultListener* listener) const;
  bool MatchAndExplain(double actual,
                       testing::MatchResultListener* listener) const;
  bool MatchAndExplain(Complexification<double> actual,
                       testing::MatchResultListener* listener) const;
  template<typename Scalar>
  bool MatchAndExplain(R3Element<Scalar> const& actual,
                       testing::MatchResultListener* listener) const;
  template<typename Scalar>
  bool MatchAndExplain(R3x3Matrix<Scalar> const& actual,
                       testing::MatchResultListener* listener) const;
  bool MatchAndExplain(Quaternion const& actual,
                       testing::MatchResultListener* listener) const;
  template<typename FromFrame, typename ToFrame>
  bool MatchAndExplain(Rotation<FromFrame, ToFrame> const& actual,
                       testing::MatchResultListener* listener) const;
  template<typename Scalar, typename Frame>
  bool MatchAndExplain(Vector<Scalar, Frame> const& actual,
                       testing::MatchResultListener* listener) const;
  template<typename Scalar, typename Frame>
  bool MatchAndExplain(Bivector<Scalar, Frame> const& actual,
                       testing::MatchResultListener* listener) const;
  template<typename Scalar, typename Frame>
  bool MatchAndExplain(Trivector<Scalar, Frame> const& actual,
                       testing::MatchResultListener* listener) const;
  template<typename Vector>
  bool MatchAndExplain(Point<Vector> const& actual,
                       testing::MatchResultListener* listener) const;
  template<typename S>
  bool MatchAndExplain(DoublePrecision<S> const& actual,
                       testing::MatchResultListener* listener) const;
  template<typename Scalar, std::int64_t size>
  bool MatchAndExplain(FixedVector<Scalar, size> const& actual,
                       testing::MatchResultListener* listener) const;
  template<typename Scalar, std::int64_t rows, std::int64_t columns>
  bool MatchAndExplain(
      FixedMatrix<Scalar, rows, columns> const& actual,
      testing::MatchResultListener* listener) const;
  template<typename Scalar, std::int64_t rows>
  bool MatchAndExplain(
      FixedLowerTriangularMatrix<Scalar, rows> const& actual,
      testing::MatchResultListener* listener) const;
  template<typename Scalar, std::int64_t columns>
  bool MatchAndExplain(
      FixedUpperTriangularMatrix<Scalar, columns> const& actual,
      testing::MatchResultListener* listener) const;
  template<typename Scalar>
  bool MatchAndExplain(UnboundedVector<Scalar> const& actual,
                       testing::MatchResultListener* listener) const;
  template<typename Scalar>
  bool MatchAndExplain(
      UnboundedMatrix<Scalar> const& actual,
      testing::MatchResultListener* listener) const;
  template<typename Scalar>
  bool MatchAndExplain(
      UnboundedLowerTriangularMatrix<Scalar> const& actual,
      testing::MatchResultListener* listener) const;
  template<typename Scalar>
  bool MatchAndExplain(
      UnboundedUpperTriangularMatrix<Scalar> const& actual,
      testing::MatchResultListener* listener) const;

  void DescribeTo(std::ostream* out) const;
  void DescribeNegationTo(std::ostream* out) const;

 private:
  bool MatchAndExplainIdentical(testing::MatchResultListener* listener) const;

  T const expected_;
  std::int64_t const min_ulps_;
  std::int64_t const max_ulps_;
};

}  // namespace internal

using internal::AlmostEquals;

}  // namespace _almost_equals
}  // namespace testing_utilities
}  // namespace principia

#include "testing_utilities/almost_equals_body.hpp"
