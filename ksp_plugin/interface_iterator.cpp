#include "ksp_plugin/interface.hpp"

#include <vector>

#include "base/not_null.hpp"
#include "geometry/rp2_point.hpp"
#include "journal/method.hpp"
#include "journal/profiles.hpp"  // 🧙 For generated profiles.
#include "ksp_plugin/frames.hpp"
#include "ksp_plugin/identification.hpp"
#include "ksp_plugin/iterators.hpp"
#include "ksp_plugin/renderer.hpp"
#include "physics/degrees_of_freedom.hpp"
#include "physics/discrete_trajectory.hpp"
#include "quantities/quantities.hpp"

namespace principia {
namespace interface {

using namespace principia::base::_not_null;
using namespace principia::geometry::_rp2_point;
using namespace principia::journal::_method;
using namespace principia::ksp_plugin::_frames;
using namespace principia::ksp_plugin::_identification;
using namespace principia::ksp_plugin::_iterators;
using namespace principia::ksp_plugin::_renderer;
using namespace principia::physics::_degrees_of_freedom;
using namespace principia::physics::_discrete_trajectory;
using namespace principia::quantities::_quantities;

bool __cdecl principia__IteratorAtEnd(Iterator const* const iterator) {
  journal::Method<journal::IteratorAtEnd> m({iterator});
  return m.Return(CHECK_NOTNULL(iterator)->AtEnd());
}

void __cdecl principia__IteratorDelete(Iterator** const iterator) {
  journal::Method<journal::IteratorDelete> m({iterator}, {iterator});
  TakeOwnership(iterator);
  return m.Return();
}

QP __cdecl principia__IteratorGetDiscreteTrajectoryQP(
    Iterator const* const iterator) {
  journal::Method<journal::IteratorGetDiscreteTrajectoryQP> m({iterator});
  CHECK_NOTNULL(iterator);
  auto const typed_iterator = check_not_null(
      dynamic_cast<TypedIterator<DiscreteTrajectory<World>> const*>(iterator));
  return m.Return(typed_iterator->Get<QP>(
      [](DiscreteTrajectory<World>::iterator const& iterator) -> QP {
        return ToQP(iterator->degrees_of_freedom);
      }));
}

Node __cdecl principia__IteratorGetNode(Iterator const* const iterator) {
  journal::Method<journal::IteratorGetNode> m({iterator});
  CHECK_NOTNULL(iterator);
  auto const typed_iterator = check_not_null(
      dynamic_cast<TypedIterator<std::vector<Renderer::Node>> const*>(
          iterator));
  auto const plugin = typed_iterator->plugin();
  return m.Return(typed_iterator->Get<Node>(
      [plugin](Renderer::Node const& node) { return ToNode(*plugin, node); }));
}

double __cdecl principia__IteratorGetDiscreteTrajectoryTime(
    Iterator const* const iterator) {
  journal::Method<journal::IteratorGetDiscreteTrajectoryTime> m({iterator});
  CHECK_NOTNULL(iterator);
  auto const typed_iterator = check_not_null(
      dynamic_cast<TypedIterator<DiscreteTrajectory<World>> const*>(iterator));
  auto const plugin = typed_iterator->plugin();
  return m.Return(typed_iterator->Get<double>(
      [plugin](DiscreteTrajectory<World>::iterator const& iterator) -> double {
        return ToGameTime(*plugin, iterator->time);
      }));
}

XYZ __cdecl principia__IteratorGetDiscreteTrajectoryXYZ(
    Iterator const* const iterator) {
  journal::Method<journal::IteratorGetDiscreteTrajectoryXYZ> m({iterator});
  CHECK_NOTNULL(iterator);
  auto const typed_iterator = check_not_null(
      dynamic_cast<TypedIterator<DiscreteTrajectory<World>> const*>(iterator));
  return m.Return(typed_iterator->Get<XYZ>(
      [](DiscreteTrajectory<World>::iterator const& iterator) -> XYZ {
        return ToXYZ(iterator->degrees_of_freedom.position());
      }));
}

Iterator* __cdecl principia__IteratorGetRP2LinesIterator(
    Iterator const* const iterator) {
  journal::Method<journal::IteratorGetRP2LinesIterator> m({iterator});
  CHECK_NOTNULL(iterator);
  auto const typed_iterator = check_not_null(
      dynamic_cast<TypedIterator<RP2Lines<Length, Camera>> const*>(iterator));
  return m.Return(typed_iterator->Get<Iterator*>(
      [](RP2Line<Length, Camera> const& rp2_line) -> Iterator* {
        return new TypedIterator<RP2Line<Length, Camera>>(rp2_line,
                                                          /*plugin=*/nullptr);
      }));
}

XY __cdecl principia__IteratorGetRP2LineXY(Iterator const* const iterator) {
  journal::Method<journal::IteratorGetRP2LineXY> m({iterator});
  CHECK_NOTNULL(iterator);
  auto const typed_iterator = check_not_null(
      dynamic_cast<TypedIterator<RP2Line<Length, Camera>> const*>(iterator));
  return m.Return(typed_iterator->Get<XY>(
      [](RP2Point<Length, Camera> const& rp2_point) -> XY {
        return ToXY(rp2_point);
      }));
}

char const* __cdecl principia__IteratorGetVesselGuid(
    Iterator const* const iterator) {
  journal::Method<journal::IteratorGetVesselGuid> m({iterator});
  auto const typed_iterator = check_not_null(
      dynamic_cast<TypedIterator<VesselSet> const*>(iterator));
  return m.Return(typed_iterator->Get<char const*>(
      [](Vessel* const vessel) -> char const* {
        return vessel->guid().c_str();
      }));
}

void __cdecl principia__IteratorIncrement(Iterator* const iterator) {
  journal::Method<journal::IteratorIncrement> m({iterator});
  CHECK_NOTNULL(iterator)->Increment();
  return m.Return();
}

void __cdecl principia__IteratorReset(Iterator* const iterator) {
  journal::Method<journal::IteratorReset> m({iterator});
  CHECK_NOTNULL(iterator)->Reset();
  return m.Return();
}

int __cdecl principia__IteratorSize(Iterator const* const iterator) {
  journal::Method<journal::IteratorSize> m({iterator});
  return m.Return(CHECK_NOTNULL(iterator)->Size());
}

}  // namespace interface
}  // namespace principia
