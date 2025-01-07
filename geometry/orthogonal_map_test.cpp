#include "geometry/orthogonal_map.hpp"

#include "geometry/frame.hpp"
#include "geometry/grassmann.hpp"
#include "geometry/identity.hpp"
#include "geometry/r3_element.hpp"
#include "geometry/rotation.hpp"
#include "geometry/signature.hpp"
#include "glog/logging.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "quantities/quantities.hpp"
#include "quantities/si.hpp"
#include "serialization/geometry.pb.h"
#include "testing_utilities/almost_equals.hpp"

namespace principia {
namespace geometry {

using ::testing::Eq;
using namespace principia::geometry::_frame;
using namespace principia::geometry::_grassmann;
using namespace principia::geometry::_identity;
using namespace principia::geometry::_orthogonal_map;
using namespace principia::geometry::_r3_element;
using namespace principia::geometry::_rotation;
using namespace principia::geometry::_signature;
using namespace principia::quantities::_quantities;
using namespace principia::quantities::_si;
using namespace principia::testing_utilities::_almost_equals;

class OrthogonalMapTest : public testing::Test {
 protected:
  using DirectWorld = Frame<serialization::Frame::TestTag,
                      Inertial,
                      Handedness::Right,
                      serialization::Frame::TEST1>;
  using MirrorWorld = Frame<serialization::Frame::TestTag,
                      Inertial,
                      Handedness::Left,
                      serialization::Frame::TEST2>;
  using DirectOrth = OrthogonalMap<DirectWorld, DirectWorld>;
  using MirrorOrth = OrthogonalMap<MirrorWorld, DirectWorld>;
  using Rot = Rotation<DirectWorld, DirectWorld>;
  using DirectSig = Signature<DirectWorld, DirectWorld>;
  using MirrorSig = Signature<MirrorWorld, DirectWorld>;

  OrthogonalMapTest()
      : direct_vector_(Vector<Length, DirectWorld>(
            R3Element<Length>(1.0 * Metre, 2.0 * Metre, 3.0 * Metre))),
        direct_bivector_(Bivector<Length, DirectWorld>(
            R3Element<Length>(1.0 * Metre, 2.0 * Metre, 3.0 * Metre))),
        direct_trivector_(Trivector<Length, DirectWorld>(4.0 * Metre)),
        mirror_vector_(Vector<Length, MirrorWorld>(
            R3Element<Length>(1.0 * Metre, 2.0 * Metre, 3.0 * Metre))),
        mirror_bivector_(Bivector<Length, MirrorWorld>(
            R3Element<Length>(1.0 * Metre, 2.0 * Metre, 3.0 * Metre))),
        mirror_trivector_(Trivector<Length, MirrorWorld>(4.0 * Metre)),
        orthogonal_a_(MirrorOrth(
            Rot(120 * Degree, Bivector<double, DirectWorld>({1, 1, 1}))
                .quaternion())),
        orthogonal_b_(DirectOrth(
            Rot(90 * Degree, Bivector<double, DirectWorld>({1, 0, 0}))
                .quaternion())),
        orthogonal_c_(MirrorOrth(
            Rot(90 * Degree, Bivector<double, DirectWorld>({1, 0, 0}))
                .quaternion())) {}

  Vector<Length, DirectWorld> direct_vector_;
  Bivector<Length, DirectWorld> direct_bivector_;
  Trivector<Length, DirectWorld> direct_trivector_;
  Vector<Length, MirrorWorld> mirror_vector_;
  Bivector<Length, MirrorWorld> mirror_bivector_;
  Trivector<Length, MirrorWorld> mirror_trivector_;
  MirrorOrth orthogonal_a_;
  DirectOrth orthogonal_b_;
  MirrorOrth orthogonal_c_;
};

using OrthogonalMapDeathTest = OrthogonalMapTest;

TEST_F(OrthogonalMapTest, Identity) {
  EXPECT_THAT(direct_vector_, Eq(DirectOrth::Identity()(direct_vector_)));
  EXPECT_THAT(direct_bivector_, Eq(DirectOrth::Identity()(direct_bivector_)));
  EXPECT_THAT(direct_trivector_, Eq(DirectOrth::Identity()(direct_trivector_)));
}

TEST_F(OrthogonalMapTest, AppliedToVector) {
  EXPECT_THAT(orthogonal_a_(mirror_vector_),
              AlmostEquals(Vector<Length, DirectWorld>(
                  R3Element<Length>(-3.0 * Metre,
                                    -1.0 * Metre,
                                    -2.0 * Metre)), 4));
  EXPECT_THAT(orthogonal_b_(direct_vector_),
              AlmostEquals(Vector<Length, DirectWorld>(
                  R3Element<Length>(1.0 * Metre,
                                    -3.0 * Metre,
                                    2.0 * Metre)), 1, 2));
}

TEST_F(OrthogonalMapTest, AppliedToBivector) {
  EXPECT_THAT(orthogonal_a_(mirror_bivector_),
              AlmostEquals(Bivector<Length, DirectWorld>(
                  R3Element<Length>(3.0 * Metre,
                                    1.0 * Metre,
                                    2.0 * Metre)), 4));
  EXPECT_THAT(orthogonal_b_(direct_bivector_),
              AlmostEquals(Bivector<Length, DirectWorld>(
                  R3Element<Length>(1.0 * Metre,
                                    -3.0 * Metre,
                                    2.0 * Metre)), 1, 2));
}

TEST_F(OrthogonalMapTest, AppliedToTrivector) {
  EXPECT_THAT(orthogonal_a_(mirror_trivector_),
              AlmostEquals(Trivector<Length, DirectWorld>(
                  -4.0 * Metre), 0));
  EXPECT_THAT(orthogonal_b_(direct_trivector_),
              AlmostEquals(Trivector<Length, DirectWorld>(
                  4.0 * Metre), 0));
}

TEST_F(OrthogonalMapTest, Determinant) {
  EXPECT_TRUE(orthogonal_a_.Determinant().is_negative());
  EXPECT_TRUE(orthogonal_b_.Determinant().is_positive());
  EXPECT_TRUE(orthogonal_c_.Determinant().is_negative());
}

TEST_F(OrthogonalMapTest, Inverse) {
  EXPECT_THAT(orthogonal_a_.Inverse()(direct_vector_),
              AlmostEquals(Vector<Length, MirrorWorld>(
                  R3Element<Length>(-2.0 * Metre,
                                    -3.0 * Metre,
                                    -1.0 * Metre)), 2));
  EXPECT_THAT(orthogonal_b_.Inverse()(direct_vector_),
              AlmostEquals(Vector<Length, DirectWorld>(
                  R3Element<Length>(1.0 * Metre,
                                    3.0 * Metre,
                                    -2.0 * Metre)), 1, 2));
}

TEST_F(OrthogonalMapTest, Composition) {
  DirectOrth const orthogonal_ac = orthogonal_a_ * orthogonal_c_.Inverse();
  EXPECT_THAT(orthogonal_ac(direct_vector_),
              AlmostEquals(Vector<Length, DirectWorld>(
                  R3Element<Length>(-2.0 * Metre,
                                    1.0 * Metre,
                                    3.0 * Metre)), 1, 6));
  EXPECT_TRUE((orthogonal_b_ * orthogonal_a_).Determinant().is_negative());
  EXPECT_TRUE((orthogonal_a_ * orthogonal_c_.Inverse()).
              Determinant().is_positive());
  EXPECT_TRUE((orthogonal_b_ * orthogonal_c_).Determinant().is_negative());
}

TEST_F(OrthogonalMapDeathTest, SerializationError) {
  Identity<DirectWorld, DirectWorld> id;
  EXPECT_DEATH({
    serialization::LinearMap message;
    id.WriteToMessage(&message);
    DirectOrth const o = DirectOrth::ReadFromMessage(message);
  }, "HasExtension.*OrthogonalMap");
}

TEST_F(OrthogonalMapTest, SerializationSuccess) {
  serialization::LinearMap message;
  orthogonal_a_.WriteToMessage(&message);
  EXPECT_TRUE(message.has_from_frame());
  EXPECT_TRUE(message.has_to_frame());
  EXPECT_EQ(message.from_frame().tag_type_fingerprint(),
            message.to_frame().tag_type_fingerprint());
  EXPECT_NE(message.from_frame().tag(),
            message.to_frame().tag());
  EXPECT_EQ(message.from_frame().is_inertial(),
            message.to_frame().is_inertial());
  EXPECT_TRUE(message.HasExtension(
      serialization::OrthogonalMap::extension));
  serialization::OrthogonalMap const& extension =
      message.GetExtension(serialization::OrthogonalMap::extension);
  EXPECT_THAT(extension.quaternion().real_part(), AlmostEquals(0.5, 1));
  EXPECT_EQ(0.5, extension.quaternion().imaginary_part().x().double_());
  EXPECT_EQ(0.5, extension.quaternion().imaginary_part().y().double_());
  EXPECT_EQ(0.5, extension.quaternion().imaginary_part().z().double_());
  MirrorOrth const o = MirrorOrth::ReadFromMessage(message);
  EXPECT_EQ(orthogonal_a_(mirror_vector_), o(mirror_vector_));
}

TEST_F(OrthogonalMapTest, Output) {
  std::cout << orthogonal_a_ << "\n";
}

}  // namespace geometry
}  // namespace principia
