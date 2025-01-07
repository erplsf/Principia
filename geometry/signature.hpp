#pragma once

#include "base/concepts.hpp"
#include "base/not_null.hpp"
#include "geometry/grassmann.hpp"
#include "geometry/linear_map.hpp"
#include "geometry/sign.hpp"

namespace principia {
namespace geometry {

FORWARD_DECLARE(
    TEMPLATE(typename Scalar,
             typename Frame,
             template<typename, typename> typename Multivector) class,
    SymmetricBilinearForm,
    FROM(symmetric_bilinear_form),
    INTO(signature));

namespace _signature {
namespace internal {

using namespace principia::base::_concepts;
using namespace principia::base::_not_null;
using namespace principia::geometry::_grassmann;
using namespace principia::geometry::_linear_map;
using namespace principia::geometry::_sign;

struct DeduceSignPreservingOrientation final {};
struct DeduceSignReversingOrientation final {};

// A coordinate change whose matrix is a signature matrix, i.e., a diagonal
// matrix with ±1 on the diagonal.  There are 8 possible signatures: the
// identity 𝟙, the central inversion -𝟙, the 180° rotations around all three
// axes, and the reflections across the planes orthogonal to all three axes.
template<typename FromFrame, typename ToFrame>
class Signature : public LinearMap<Signature<FromFrame, ToFrame>,
                                   FromFrame, ToFrame> {
 public:
  using DeduceSign =
      std::conditional_t<FromFrame::handedness == ToFrame::handedness,
                         DeduceSignPreservingOrientation,
                         DeduceSignReversingOrientation>;

  constexpr Signature(Sign x, Sign y, DeduceSign z);
  constexpr Signature(Sign x, DeduceSign y, Sign z);
  constexpr Signature(DeduceSign x, Sign y, Sign z);

  constexpr Sign Determinant() const;

  template<typename F = FromFrame,
           typename T = ToFrame,
           typename = std::enable_if_t<F::handedness == T::handedness>>
  static constexpr Signature Identity();

  template<typename F = FromFrame,
           typename T = ToFrame,
           typename = std::enable_if_t<F::handedness != T::handedness>>
  static constexpr Signature CentralInversion();

  constexpr Signature<ToFrame, FromFrame> Inverse() const;

  template<typename Scalar>
  Vector<Scalar, ToFrame> operator()(
      Vector<Scalar, FromFrame> const& vector) const;

  template<typename Scalar>
  Bivector<Scalar, ToFrame> operator()(
      Bivector<Scalar, FromFrame> const& bivector) const;

  template<typename Scalar>
  Trivector<Scalar, ToFrame> operator()(
      Trivector<Scalar, FromFrame> const& trivector) const;

  template<typename Scalar,
           template<typename, typename> typename Multivector>
  SymmetricBilinearForm<Scalar, ToFrame, Multivector> operator()(
      SymmetricBilinearForm<Scalar, FromFrame, Multivector> const& form) const;

  template<template<typename, typename> typename LinearMap>
  LinearMap<FromFrame, ToFrame> Forget() const;
  template<template<typename, typename, typename> typename ConformalMap>
  ConformalMap<double, FromFrame, ToFrame> Forget() const;

  void WriteToMessage(not_null<serialization::LinearMap*> message) const;
  static Signature ReadFromMessage(serialization::LinearMap const& message)
    requires serializable<FromFrame> && serializable<ToFrame>;

  void WriteToMessage(not_null<serialization::Signature*> message) const;
  static Signature ReadFromMessage(serialization::Signature const& message)
    requires serializable<FromFrame> && serializable<ToFrame>;

 private:
  constexpr Signature(Sign x, Sign y, Sign z);

  Sign x_;
  Sign y_;
  Sign z_;

  static constexpr Sign determinant_ =
      FromFrame::handedness == ToFrame::handedness ? Sign::Positive()
                                                   : Sign::Negative();

  template<typename From, typename To>
  friend class Signature;

  template<typename From, typename Through, typename To>
  friend Signature<From, To> operator*(Signature<Through, To> const& left,
                                       Signature<From, Through> const& right);

  template<typename From, typename To>
  friend std::ostream& operator<<(std::ostream& out,
                                  Signature<From, To> const& signature);
};

template<typename FromFrame, typename ThroughFrame, typename ToFrame>
Signature<FromFrame, ToFrame> operator*(
    Signature<ThroughFrame, ToFrame> const& left,
    Signature<FromFrame, ThroughFrame> const& right);

template<typename FromFrame, typename ToFrame>
std::ostream& operator<<(std::ostream& out,
                         Signature<FromFrame, ToFrame> const& signature);

}  // namespace internal

using internal::DeduceSignPreservingOrientation;
using internal::DeduceSignReversingOrientation;
using internal::Signature;

}  // namespace _signature
}  // namespace geometry
}  // namespace principia

#include "geometry/signature_body.hpp"
