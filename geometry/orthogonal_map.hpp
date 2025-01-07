#pragma once

#include "base/concepts.hpp"
#include "base/mappable.hpp"
#include "base/not_null.hpp"
#include "geometry/frame.hpp"
#include "geometry/grassmann.hpp"
#include "geometry/linear_map.hpp"
#include "geometry/quaternion.hpp"
#include "geometry/sign.hpp"
#include "serialization/geometry.pb.h"

namespace principia {
namespace geometry {

FORWARD_DECLARE(TEMPLATE(typename Scalar,
                         typename FromFrame,
                         typename ToFrame) class,
                ConformalMap,
                FROM(conformal_map));
FORWARD_DECLARE(TEMPLATE(typename FromFrame, typename ToFrame) class,
                Identity,
                FROM(identity),
                INTO(orthogonal_map));
FORWARD_DECLARE(TEMPLATE(typename FromFrame, typename ToFrame) class,
                Permutation,
                FROM(permutation),
                INTO(orthogonal_map));
FORWARD_DECLARE(TEMPLATE(typename FromFrame, typename ToFrame) class,
                Rotation,
                FROM(rotation),
                INTO(orthogonal_map));
FORWARD_DECLARE(TEMPLATE(typename FromFrame, typename ToFrame) class,
                Signature,
                FROM(signature),
                INTO(orthogonal_map));
FORWARD_DECLARE(
    TEMPLATE(typename Scalar,
             typename Frame,
             template<typename, typename> typename Multivector) class,
    SymmetricBilinearForm,
    FROM(symmetric_bilinear_form),
    INTO(orthogonal_map));

class ConformalMapTest;
class OrthogonalMapTest;

namespace _orthogonal_map {
namespace internal {

using namespace principia::base::_concepts;
using namespace principia::base::_mappable;
using namespace principia::base::_not_null;
using namespace principia::geometry::_frame;
using namespace principia::geometry::_grassmann;
using namespace principia::geometry::_linear_map;
using namespace principia::geometry::_quaternion;
using namespace principia::geometry::_sign;

// An orthogonal map between the inner product spaces `FromFrame` and
// `ToFrame`, as well as the induced maps on the exterior algebra.
// The orthogonal map is modeled as a rotoinversion.
template<typename FromFrame, typename ToFrame>
class OrthogonalMap : public LinearMap<OrthogonalMap<FromFrame, ToFrame>,
                                       FromFrame, ToFrame> {
 public:
  Sign Determinant() const;

  template<typename F = FromFrame,
           typename T = ToFrame,
           typename = std::enable_if_t<F::handedness == T::handedness>>
  Rotation<FromFrame, ToFrame> AsRotation() const;

  OrthogonalMap<ToFrame, FromFrame> Inverse() const;

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

  template<typename T>
  typename Mappable<OrthogonalMap, T>::type operator()(T const& t) const;

  template<template<typename, typename> typename ConformalMap>
  ConformalMap<FromFrame, ToFrame> Forget() const;
  template<template<typename, typename, typename> typename ConformalMap>
  ConformalMap<double, FromFrame, ToFrame> Forget() const;

  template<typename F = FromFrame,
           typename T = ToFrame,
           typename = std::enable_if_t<F::handedness == T::handedness>>
  static OrthogonalMap Identity();

  void WriteToMessage(not_null<serialization::LinearMap*> message) const;
  static OrthogonalMap ReadFromMessage(serialization::LinearMap const& message)
    requires serializable<FromFrame> && serializable<ToFrame>;

  void WriteToMessage(not_null<serialization::OrthogonalMap*> message) const;
  static OrthogonalMap ReadFromMessage(
      serialization::OrthogonalMap const& message)
    requires serializable<FromFrame> && serializable<ToFrame>;

 private:
  explicit OrthogonalMap(Quaternion const& quaternion);

  using IntermediateFrame = Frame<struct IntermediateFrameTag,
                                  ToFrame::motion,
                                  ToFrame::handedness>;

  static constexpr Signature<FromFrame, IntermediateFrame> MakeSignature();
  Rotation<IntermediateFrame, ToFrame> MakeRotation() const;

  Quaternion quaternion_;

  static constexpr Sign determinant_ =
      FromFrame::handedness == ToFrame::handedness ? Sign::Positive()
                                                   : Sign::Negative();

  template<typename From, typename To>
  friend class OrthogonalMap;
  template<typename Scalar, typename From, typename To>
  friend class _conformal_map::ConformalMap;
  template<typename From, typename To>
  friend class _identity::Identity;
  template<typename From, typename To>
  friend class _permutation::Permutation;
  template<typename From, typename To>
  friend class _rotation::Rotation;
  template<typename From, typename To>
  friend class _signature::Signature;

  template<typename From, typename Through, typename To>
  friend OrthogonalMap<From, To> operator*(
      OrthogonalMap<Through, To> const& left,
      OrthogonalMap<From, Through> const& right);

  template<typename From, typename To>
  friend std::ostream& operator<<(
      std::ostream& out,
      OrthogonalMap<From, To> const& orthogonal_map);

  friend class geometry::ConformalMapTest;
  friend class geometry::OrthogonalMapTest;
};

template<typename FromFrame, typename ThroughFrame, typename ToFrame>
OrthogonalMap<FromFrame, ToFrame> operator*(
    OrthogonalMap<ThroughFrame, ToFrame> const& left,
    OrthogonalMap<FromFrame, ThroughFrame> const& right);

template<typename FromFrame, typename ToFrame>
std::ostream& operator<<(
    std::ostream& out,
    OrthogonalMap<FromFrame, ToFrame> const& orthogonal_map);

}  // namespace internal

using internal::OrthogonalMap;

}  // namespace _orthogonal_map
}  // namespace geometry
}  // namespace principia

#include "geometry/orthogonal_map_body.hpp"
