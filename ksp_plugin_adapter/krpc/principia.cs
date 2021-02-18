using System;
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
                                      prms.integrator_kind,
                                      prms.generalized_integrator_kind,
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
  private readonly string vessel_guid_;
  private readonly long integrator_kind_;
  private readonly long generalized_integrator_kind_;
  private long max_steps_;
  private double length_integration_tolerance_;
  private double speed_integration_tolerance_;

  public FlightPlanParameters(string vessel_guid,
                              long integrator_kind,
                              long generalized_integrator_kind,
                              long max_steps,
                              double length_integration_tolerance,
                              double speed_integration_tolerance) {
    vessel_guid_ = vessel_guid;
    integrator_kind_ = integrator_kind;
    generalized_integrator_kind_ = generalized_integrator_kind;
    max_steps_ = max_steps;
    length_integration_tolerance_ = length_integration_tolerance;
    speed_integration_tolerance_ = speed_integration_tolerance;
  }

  private void Commit() {
    Principia.plugin_.FlightPlanSetAdaptiveStepParameters(vessel_guid_,
      new FlightPlanAdaptiveStepParameters{
          integrator_kind = integrator_kind_,
          generalized_integrator_kind = generalized_integrator_kind_,
          max_steps = max_steps_,
          length_integration_tolerance = length_integration_tolerance_,
          speed_integration_tolerance = speed_integration_tolerance_
      });
  }

  [KRPCProperty]
  public double Tolerance {
    get => length_integration_tolerance_;
    set  {
      if (value >= 1e-6 && value <= 1e6) {
        length_integration_tolerance_ = value;
        speed_integration_tolerance_ = value;
        Commit();
      } else {
        throw new Exception("Tolerance is out of the supported range");
      }
    }
  }

  [KRPCProperty]
  public long MaxSteps {
    get => max_steps_;
    set {
      if (value >= 100 && value <= long.MaxValue / factor) {
        max_steps_ = value;
        Commit();
      } else {
        throw new Exception("Max steps is out of the supported range");
      }
    }
  }

  private const int factor = 4;
}

}
