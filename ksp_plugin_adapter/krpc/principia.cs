using System;
using KRPC.Service;
using KRPC.Service.Attributes;

namespace principia.ksp_plugin_adapter.krpc {

[KRPCService (GameScene = GameScene.Flight)]
public static class Principia {
  private static readonly IntPtr plugin_;

  static Principia() {
    plugin_ = ExternalInterface.Get().adapter_.Plugin();
  }

  [KRPCProcedure]
  public static bool FlightPlanExists() {
    return plugin_.FlightPlanExists(FlightGlobals.ActiveVessel.id.ToString());
  }

  [KRPCProcedure]
  public static void GetOrCreateFlightPlan() {
    Vessel vessel = FlightGlobals.ActiveVessel;
    if (!plugin_.FlightPlanExists(vessel.id.ToString())) {
      plugin_.FlightPlanCreate(vessel.id.ToString(),
                               plugin_.CurrentTime() + 3600,
                               vessel.GetTotalMass());
    }
  }

  [KRPCProcedure (Nullable = true)]
  public static FlightPlanParameters GetFlightParameters() {
    string vessel_guid = FlightGlobals.ActiveVessel.id.ToString();
    if (plugin_.FlightPlanExists(vessel_guid)) {
      FlightPlanAdaptiveStepParameters prms =
          plugin_.FlightPlanGetAdaptiveStepParameters(vessel_guid);
      return new FlightPlanParameters(vessel_guid,
                                      prms.max_steps,
                                      prms.length_integration_tolerance,
                                      prms.speed_integration_tolerance);
    } else {
      return null;
    }
  }
}

[KRPCClass (Service = "Principia")]
public class FlightPlanParameters {
  private string vessel_guid_;

  public FlightPlanParameters(string vessel_guid,
                              long max_steps,
                              double length_integration_tolerance,
                              double speed_integration_tolerance) {
    vessel_guid_ = vessel_guid;
    this.MaxSteps = max_steps;
    this.LengthIntegrationTolerance = length_integration_tolerance;
    this.SpeedIntegrationTolerance = speed_integration_tolerance;
  }

  [KRPCProperty]
  public long MaxSteps { get ; }

  [KRPCProperty]
  public double LengthIntegrationTolerance { get ; }

  [KRPCProperty]
  public double SpeedIntegrationTolerance { get ; }
}

}
