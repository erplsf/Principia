using System;
using JetBrains.Annotations;
using KRPC.Service;
using KRPC.Service.Attributes;

// ReSharper disable InconsistentNaming

namespace principia.ksp_plugin_adapter.krpc {

[KRPCService (GameScene = GameScene.Flight)]
public static class Principia {
  internal static readonly IntPtr plugin_;

  static Principia() {
    plugin_ = ExternalInterface.Get().adapter_.Plugin();
  }

  [KRPCProcedure]
  [UsedImplicitly]
  public static FlightPlan GetOrCreateFlightPlan() {
    Vessel vessel = FlightGlobals.ActiveVessel;
    string vessel_guid = vessel.id.ToString();
    if (!plugin_.FlightPlanExists(vessel_guid)) {
      plugin_.FlightPlanCreate(vessel_guid,
                               plugin_.CurrentTime() + 3600,
                               vessel.GetTotalMass());
    }
    FlightPlanAdaptiveStepParameters prms =
        plugin_.FlightPlanGetAdaptiveStepParameters(vessel_guid);
    double final_time = plugin_.FlightPlanGetDesiredFinalTime(vessel_guid);

    return new FlightPlan(vessel_guid,
                          final_time,
                          prms.integrator_kind,
                          prms.generalized_integrator_kind,
                          prms.max_steps,
                          prms.length_integration_tolerance,
                          prms.speed_integration_tolerance);
  }

  [KRPCProcedure]
  [UsedImplicitly]
  public static void DeleteFlightPlan() {
    string vessel_guid = FlightGlobals.ActiveVessel.id.ToString();
    if (plugin_.FlightPlanExists(vessel_guid)) {
      plugin_.FlightPlanDelete(vessel_guid);
    }
  }
}

}
