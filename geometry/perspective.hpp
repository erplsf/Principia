﻿
#pragma once

#include "geometry/affine_map.hpp"
#include "geometry/grassmann.hpp"
#include "geometry/point.hpp"
#include "geometry/rp2_point.hpp"
#include "geometry/sphere.hpp"

namespace principia {
namespace geometry {
namespace internal_perspective {

// A perspective using the pinhole camera model.  It project a point of
// |FromFrame| to an element of ℝP².  |ToFrame| is the frame of the camera.  In
// that frame the camera is located at the origin and looking at the positive
// z-axis.  The x- and y- axis of the camera correspond to those of ℝP².
template<typename FromFrame, typename ToFrame, typename Scalar,
         template<typename, typename> class LinearMap>
class Perspective final {
 public:
  Perspective(
      AffineMap<ToFrame, FromFrame, Scalar, LinearMap> const& from_camera,
      Scalar const& focal);
  Perspective(
      AffineMap<FromFrame, ToFrame, Scalar, LinearMap> const& to_camera,
      Scalar const& focal);

  RP2Point<Scalar, ToFrame> operator()(
      Point<Vector<Scalar, FromFrame>> const& point) const;

  bool IsHiddenBySphere(Point<Vector<Scalar, FromFrame>> const& point,
                        Sphere<Scalar, FromFrame> const& sphere) const;

 private:
  AffineMap<ToFrame, FromFrame, Scalar, LinearMap> const from_camera_;
  AffineMap<FromFrame, ToFrame, Scalar, LinearMap> const to_camera_;
  Point<Vector<Scalar, FromFrame>> const camera_;
  Scalar const focal_;
};

}  // namespace internal_perspective

using internal_perspective::Perspective;

}  // namespace geometry
}  // namespace principia

#include "geometry/perspective_body.hpp"