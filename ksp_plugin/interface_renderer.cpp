#include "ksp_plugin/interface.hpp"

#include <utility>
#include <vector>

#include "journal/method.hpp"
#include "journal/profiles.hpp"  // 🧙 For generated profiles.
#include "ksp_plugin/iterators.hpp"
#include "ksp_plugin/renderer.hpp"
#include "physics/discrete_trajectory.hpp"

namespace principia {
namespace interface {

using namespace principia::journal::_method;
using namespace principia::ksp_plugin::_iterators;
using namespace principia::ksp_plugin::_renderer;
using namespace principia::physics::_discrete_trajectory;

namespace {

Renderer& GetRenderer(Plugin* const plugin) {
  return CHECK_NOTNULL(plugin)->renderer();
}

[[maybe_unused]] Renderer const& GetRenderer(Plugin const* const plugin) {
  return CHECK_NOTNULL(plugin)->renderer();
}

}  // namespace

void __cdecl principia__ClearTargetVessel(Plugin* const plugin) {
  journal::Method<journal::ClearTargetVessel> m({plugin});
  GetRenderer(plugin).ClearTargetVessel();
  return m.Return();
}

void __cdecl principia__RenderedPredictionApsides(
    Plugin const* const plugin,
    char const* const vessel_guid,
    double const* const t_max,
    int const celestial_index,
    XYZ const sun_world_position,
    int const max_points,
    Iterator** const apoapsides,
    Iterator** const periapsides) {
  journal::Method<journal::RenderedPredictionApsides> m(
      {plugin,
       vessel_guid,
       t_max,
       celestial_index,
       sun_world_position,
       max_points},
      {apoapsides, periapsides});
  CHECK_NOTNULL(plugin);
  auto const prediction = plugin->GetVessel(vessel_guid)->prediction();
  DiscreteTrajectory<World> rendered_apoapsides;
  DiscreteTrajectory<World> rendered_periapsides;
  plugin->ComputeAndRenderApsides(
      celestial_index,
      *prediction,
      prediction->begin(), prediction->end(),
      t_max == nullptr ? InfiniteFuture : FromGameTime(*plugin, *t_max),
      FromXYZ<Position<World>>(sun_world_position),
      max_points,
      rendered_apoapsides,
      rendered_periapsides);
  *apoapsides = new TypedIterator<DiscreteTrajectory<World>>(
      std::move(rendered_apoapsides),
      plugin);
  *periapsides = new TypedIterator<DiscreteTrajectory<World>>(
      std::move(rendered_periapsides),
      plugin);
  return m.Return();
}

void __cdecl principia__RenderedPredictionClosestApproaches(
    Plugin const* const plugin,
    char const* const vessel_guid,
    XYZ const sun_world_position,
    int const max_points,
    Iterator** const closest_approaches) {
  journal::Method<journal::RenderedPredictionClosestApproaches> m(
      {plugin, vessel_guid, sun_world_position, max_points},
      {closest_approaches});
  CHECK_NOTNULL(plugin);
  auto const prediction = plugin->GetVessel(vessel_guid)->prediction();
  DiscreteTrajectory<World> rendered_closest_approaches;
  plugin->ComputeAndRenderClosestApproaches(
      *prediction,
      prediction->begin(),
      prediction->end(),
      FromXYZ<Position<World>>(sun_world_position),
      max_points,
      rendered_closest_approaches);
  *closest_approaches = new TypedIterator<DiscreteTrajectory<World>>(
      std::move(rendered_closest_approaches),
      plugin);
  return m.Return();
}

void __cdecl principia__RenderedPredictionNodes(Plugin const* const plugin,
                                                char const* const vessel_guid,
                                                double const* const t_max,
                                                XYZ const sun_world_position,
                                                int const max_points,
                                                Iterator** const ascending,
                                                Iterator** const descending) {
  journal::Method<journal::RenderedPredictionNodes> m(
      {plugin, vessel_guid, t_max, sun_world_position, max_points},
      {ascending, descending});
  CHECK_NOTNULL(plugin);
  auto const prediction = plugin->GetVessel(vessel_guid)->prediction();
  std::vector<Renderer::Node> rendered_ascending;
  std::vector<Renderer::Node> rendered_descending;
  plugin->ComputeAndRenderNodes(
      prediction->begin(), prediction->end(),
      t_max == nullptr ? InfiniteFuture : FromGameTime(*plugin, *t_max),
      FromXYZ<Position<World>>(sun_world_position),
      max_points,
      rendered_ascending,
      rendered_descending);
  *ascending = new TypedIterator<std::vector<Renderer::Node>>(
      std::move(rendered_ascending), plugin);
  *descending = new TypedIterator<std::vector<Renderer::Node>>(
      std::move(rendered_descending), plugin);
  return m.Return();
}

// Calls `plugin` to create a `NavigationFrame` using the given `parameters`,
// sets it as the current plotting frame.
void __cdecl principia__SetPlottingFrame(
    Plugin* const plugin,
    PlottingFrameParameters const& parameters) {
  journal::Method<journal::SetPlottingFrame> m({plugin, parameters});
  auto navigation_frame = NewPlottingFrame(*plugin, parameters);
  GetRenderer(plugin).SetPlottingFrame(std::move(navigation_frame));
  return m.Return();
}

void __cdecl principia__SetTargetVessel(Plugin* const plugin,
                                        char const* const vessel_guid,
                                        int const reference_body_index) {
  journal::Method<journal::SetTargetVessel> m(
      {plugin, vessel_guid, reference_body_index});
  CHECK_NOTNULL(plugin);
  plugin->SetTargetVessel(vessel_guid, reference_body_index);
  return m.Return();
}

WXYZ __cdecl principia__CameraReferenceRotation(Plugin* const plugin) {
  journal::Method<journal::CameraReferenceRotation> m({plugin});
  CHECK_NOTNULL(plugin);
  return m.Return(
      ToWXYZ(GetRenderer(plugin)
                 .CameraReferenceRotation(plugin->CurrentTime(),
                                          plugin->PlanetariumRotation(),
                                          plugin->CameraCompensation())
                 .quaternion()));
}

double __cdecl principia__CameraScale(Plugin* const plugin) {
  journal::Method<journal::CameraScale> m({plugin});
  CHECK_NOTNULL(plugin);
  return m.Return(GetRenderer(plugin)
                      .BarycentricToPlotting(plugin->CurrentTime())
                      .conformal_map()
                      .scale());
}

}  // namespace interface
}  // namespace principia
