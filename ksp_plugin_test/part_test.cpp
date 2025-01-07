#include "ksp_plugin/part.hpp"

#include "astronomy/epoch.hpp"
#include "base/not_null.hpp"
#include "geometry/grassmann.hpp"
#include "geometry/instant.hpp"
#include "geometry/r3x3_matrix.hpp"
#include "geometry/space.hpp"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "ksp_plugin/frames.hpp"
#include "ksp_plugin/identification.hpp"
#include "ksp_plugin/pile_up.hpp"
#include "physics/degrees_of_freedom.hpp"
#include "physics/rigid_motion.hpp"
#include "physics/tensors.hpp"
#include "quantities/named_quantities.hpp"
#include "quantities/quantities.hpp"
#include "quantities/si.hpp"
#include "testing_utilities/almost_equals.hpp"
#include "testing_utilities/matchers.hpp"

namespace principia {
namespace ksp_plugin {

using ::testing::_;
using ::testing::MockFunction;
using namespace principia::astronomy::_epoch;
using namespace principia::base::_not_null;
using namespace principia::geometry::_grassmann;
using namespace principia::geometry::_instant;
using namespace principia::geometry::_r3x3_matrix;
using namespace principia::geometry::_space;
using namespace principia::ksp_plugin::_frames;
using namespace principia::ksp_plugin::_identification;
using namespace principia::ksp_plugin::_part;
using namespace principia::ksp_plugin::_pile_up;
using namespace principia::physics::_degrees_of_freedom;
using namespace principia::physics::_rigid_motion;
using namespace principia::physics::_tensors;
using namespace principia::quantities::_named_quantities;
using namespace principia::quantities::_quantities;
using namespace principia::quantities::_si;
using namespace principia::testing_utilities::_almost_equals;
using namespace principia::testing_utilities::_matchers;

class PartTest : public testing::Test {
 protected:
  PartTest()
      : inertia_tensor_(MakeWaterSphereInertiaTensor(mass_)),
        part_(part_id_,
              "part",
              mass_,
              EccentricPart::origin,
              inertia_tensor_,
              RigidMotion<EccentricPart, Barycentric>::MakeNonRotatingMotion(
                  degrees_of_freedom_),
              /*deletion_callback=*/nullptr) {
    part_.apply_intrinsic_force(intrinsic_force_);
    part_.AppendToHistory(
        J2000,
        {Barycentric::origin +
             Displacement<Barycentric>({11 * Metre, 22 * Metre, 33 * Metre}),
         Velocity<Barycentric>(
             {44 * Metre / Second, 55 * Metre / Second, 66 * Metre / Second})});
  }

  DegreesOfFreedom<Barycentric> const degrees_of_freedom_ = {
      Barycentric::origin +
          Displacement<Barycentric>({1 * Metre, 2 * Metre, 3 * Metre}),
      Velocity<Barycentric>(
          {4 * Metre / Second, 5 * Metre / Second, 6 * Metre / Second})};
  PartId const part_id_ = 666;
  Mass const mass_ = 7 * Kilogram;
  Vector<Force, Barycentric> const intrinsic_force_ =
      Vector<Force, Barycentric>({8 * Newton, 9 * Newton, 10 * Newton});
  InertiaTensor<RigidPart> inertia_tensor_;
  Part part_;
};

TEST_F(PartTest, Serialization) {
  MockFunction<int(not_null<not_null<PileUp const*>>)>
      serialization_index_for_pile_up;
  EXPECT_CALL(serialization_index_for_pile_up, Call(_)).Times(0);

  serialization::Part message;
  part_.WriteToMessage(&message,
                       serialization_index_for_pile_up.AsStdFunction());
  EXPECT_EQ(part_id_, message.part_id());
  EXPECT_TRUE(message.has_inertia_tensor());
  EXPECT_TRUE(message.has_mass());
  EXPECT_EQ(7, message.mass().magnitude());
  EXPECT_TRUE(message.has_intrinsic_force());
  EXPECT_TRUE(message.intrinsic_force().has_vector());
  EXPECT_EQ(8, message.intrinsic_force().vector().x().quantity().magnitude());
  EXPECT_EQ(9, message.intrinsic_force().vector().y().quantity().magnitude());
  EXPECT_EQ(10, message.intrinsic_force().vector().z().quantity().magnitude());
  EXPECT_TRUE(message.has_rigid_motion());
  EXPECT_TRUE(message.rigid_motion().has_rigid_transformation());
  EXPECT_TRUE(message.rigid_motion().rigid_transformation().has_to_origin());
  EXPECT_TRUE(message.rigid_motion()
                  .rigid_transformation()
                  .to_origin()
                  .has_multivector());
  EXPECT_TRUE(message.rigid_motion()
                  .rigid_transformation()
                  .to_origin()
                  .multivector()
                  .has_vector());
  EXPECT_EQ(1,
            message.rigid_motion()
                .rigid_transformation()
                .to_origin()
                .multivector()
                .vector()
                .x()
                .quantity()
                .magnitude());
  EXPECT_EQ(2,
            message.rigid_motion()
                .rigid_transformation()
                .to_origin()
                .multivector()
                .vector()
                .y()
                .quantity()
                .magnitude());
  EXPECT_EQ(3,
            message.rigid_motion()
                .rigid_transformation()
                .to_origin()
                .multivector()
                .vector()
                .z()
                .quantity()
                .magnitude());
  EXPECT_TRUE(
      message.rigid_motion().velocity_of_to_frame_origin().has_vector());
  EXPECT_THAT(message.rigid_motion()
                  .velocity_of_to_frame_origin()
                  .vector()
                  .x()
                  .quantity()
                  .magnitude(),
              AlmostEquals(-4, 6));
  EXPECT_THAT(message.rigid_motion()
                  .velocity_of_to_frame_origin()
                  .vector()
                  .y()
                  .quantity()
                  .magnitude(),
              AlmostEquals(-6, 2));
  EXPECT_THAT(message.rigid_motion()
                  .velocity_of_to_frame_origin()
                  .vector()
                  .z()
                  .quantity()
                  .magnitude(),
              AlmostEquals(-5, 2));
  EXPECT_EQ(1, message.prehistory().segment_size());
  EXPECT_EQ(1, message.prehistory().segment(0).zfp().timeline_size());

  auto const p = Part::ReadFromMessage(message, /*deletion_callback=*/nullptr);
  EXPECT_EQ(part_.inertia_tensor(), p->inertia_tensor());
  EXPECT_EQ(part_.intrinsic_force(), p->intrinsic_force());
  EXPECT_EQ(part_.rigid_motion()({RigidPart::origin, RigidPart::unmoving}),
            p->rigid_motion()({RigidPart::origin, RigidPart::unmoving}));
  EXPECT_EQ(part_.rigid_motion().angular_velocity_of<RigidPart>(),
            p->rigid_motion().angular_velocity_of<RigidPart>());

  serialization::Part second_message;
  p->WriteToMessage(&second_message,
                    serialization_index_for_pile_up.AsStdFunction());
  EXPECT_THAT(message, EqualsProto(second_message));
}

}  // namespace ksp_plugin
}  // namespace principia
