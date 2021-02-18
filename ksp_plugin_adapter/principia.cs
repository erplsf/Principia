using System;
using KRPC.Service;
using KRPC.Service.Attributes;

namespace principia.ksp_plugin_adapter {

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
    var vessel = FlightGlobals.ActiveVessel;
    if (!plugin_.FlightPlanExists(vessel.id.ToString()))
    {
      plugin_.FlightPlanCreate(vessel.id.ToString(),
                               plugin_.CurrentTime() + 3600,
                               vessel.GetTotalMass());
    }
  }
}

}
