#include "ksp_plugin/plugin.hpp"

#include <string>
#include <vector>

#include "base/pull_serializer.hpp"
#include "base/push_deserializer.hpp"
#include "base/serialization.hpp"
#include "benchmark/benchmark.h"
#include "gtest/gtest.h"
#include "ksp_plugin/identification.hpp"
#include "ksp_plugin/interface.hpp"  // 🧙 For interfacing functions.
#include "ksp_plugin/iterators.hpp"
#include "ksp_plugin/pile_up.hpp"
#include "ksp_plugin_test/plugin_io.hpp"
#include "quantities/named_quantities.hpp"
#include "quantities/quantities.hpp"
#include "quantities/si.hpp"
#include "serialization/ksp_plugin.pb.h"
#include "testing_utilities/serialization.hpp"

namespace principia {
namespace ksp_plugin {
namespace _benchmark {
namespace internal {

using interface::principia__AdvanceTime;
using interface::principia__FutureCatchUpVessel;
using interface::principia__FutureWaitForVesselToCatchUp;
using interface::principia__IteratorDelete;
using interface::principia__SerializePlugin;
using namespace principia::base::_pull_serializer;
using namespace principia::base::_push_deserializer;
using namespace principia::base::_serialization;
using namespace principia::ksp_plugin::_identification;
using namespace principia::ksp_plugin::_iterators;
using namespace principia::ksp_plugin::_pile_up;
using namespace principia::ksp_plugin::_plugin;
using namespace principia::ksp_plugin_test::_plugin_io;
using namespace principia::quantities::_named_quantities;
using namespace principia::quantities::_quantities;
using namespace principia::quantities::_si;
using namespace principia::testing_utilities::_serialization;

void BM_PluginIntegrationBenchmark(benchmark::State& state) {
  auto const plugin = Plugin::ReadFromMessage(
      ParseFromBytes<serialization::Plugin>(ReadFromBinaryFile(
          SOLUTION_DIR / "ksp_plugin_test" / "3 vessels.proto.bin")));

  std::vector<GUID> const vessel_guids = {
      "70ff8dc0-a4dd-4b8c-868b-35ddb01e32bc",
      "abd95a7e-6b8b-4dba-a1e9-c96cd594cd67",
      "b86d2efd-5150-4a44-8c36-04820a85e861"};

  static constexpr int warp_factor = 6'000'000;
  static constexpr Frequency refresh_frequency = 50 * Hertz;
  static constexpr Time step = warp_factor / refresh_frequency;
  for (auto _ : state) {
    principia__AdvanceTime(
        plugin.get(),
        (plugin->CurrentTime() + step - plugin->GameEpoch()) / Second,
        /*planetarium_rotation=*/45);
    std::vector<PileUpFuture*> futures;
    for (GUID const& vessel_guid : vessel_guids) {
      futures.push_back(
          principia__FutureCatchUpVessel(plugin.get(), vessel_guid.c_str()));
    }
    for (auto& future : futures) {
      Iterator* iterator;
      principia__FutureWaitForVesselToCatchUp(plugin.get(),
                                              &future,
                                              &iterator);
      principia__IteratorDelete(&iterator);
    }
  }
}

void BM_PluginSerializationBenchmark(benchmark::State& state) {
  char const compressor[] = "gipfeli";
  char const encoder[] = "hexadecimal";

  // First, construct a plugin by reading a file.
  auto const plugin = ReadPluginFromFile(
      SOLUTION_DIR / "ksp_plugin_test" / "large_plugin.proto.gipfeli.hex",
      compressor,
      encoder);

  std::int64_t bytes_processed = 0;
  for (auto _ : state) {
    PullSerializer* serializer = nullptr;
    char const* serialization = nullptr;
    for (;;) {
      serialization = principia__SerializePlugin(plugin.get(),
                                                 &serializer,
                                                 compressor,
                                                 encoder);
      if (serialization == nullptr) {
        break;
      }
      bytes_processed += std::strlen(serialization);
      delete serialization;
    }
  }

  state.SetBytesProcessed(bytes_processed);
}

void BM_PluginDeserializationBenchmark(benchmark::State& state) {
  char const compressor[] = "gipfeli";
  char const encoder[] = "hexadecimal";

  std::int64_t bytes_processed = 0;
  for (auto _ : state) {
    auto const plugin = ReadPluginFromFile(
        SOLUTION_DIR / "ksp_plugin_test" / "large_plugin.proto.gipfeli.hex",
        compressor,
        encoder,
        bytes_processed);
    benchmark::DoNotOptimize(plugin);
  }
  state.SetBytesProcessed(bytes_processed);
}

BENCHMARK(BM_PluginSerializationBenchmark)->Unit(benchmark::kMillisecond);
BENCHMARK(BM_PluginDeserializationBenchmark)->Unit(benchmark::kMillisecond);
BENCHMARK(BM_PluginIntegrationBenchmark)->Unit(benchmark::kMillisecond);

// .\Release\x64\ksp_plugin_test_tests.exe --gtest_filter=PluginBenchmark.DISABLED_All --gtest_also_run_disabled_tests  // NOLINT
TEST(PluginBenchmark, DISABLED_All) {
  benchmark::RunSpecifiedBenchmarks();
}

}  // namespace internal
}  // namespace _benchmark
}  // namespace ksp_plugin
}  // namespace principia
