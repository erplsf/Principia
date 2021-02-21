using System;
using JetBrains.Annotations;
using KRPC.Service.Attributes;

namespace principia.ksp_plugin_adapter.krpc {

[KRPCClass (Service = "Principia")]
public class FlightPlan {
  private readonly string vessel_guid_;
  private readonly long integrator_kind_;
  private readonly long generalized_integrator_kind_;
  private long max_steps_;
  private double length_integration_tolerance_;
  private double speed_integration_tolerance_;
  private double final_time_;

  public FlightPlan(string vessel_guid,
                    double final_time,
                    long integrator_kind,
                    long generalized_integrator_kind,
                    long max_steps,
                    double length_integration_tolerance,
                    double speed_integration_tolerance) {
    final_time_ = final_time;
    vessel_guid_ = vessel_guid;
    integrator_kind_ = integrator_kind;
    generalized_integrator_kind_ = generalized_integrator_kind;
    max_steps_ = max_steps;
    length_integration_tolerance_ = length_integration_tolerance;
    speed_integration_tolerance_ = speed_integration_tolerance;
  }

  private void Commit() {
    Principia.plugin_.FlightPlanSetDesiredFinalTime(vessel_guid_, final_time_);
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
  [UsedImplicitly]
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
  [UsedImplicitly]
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

  [KRPCProperty]
  [UsedImplicitly]
  public double FinalTime {
    get => final_time_;
    set {
      if (value >= 10 && value <= double.PositiveInfinity) {
        final_time_ = value;
        Commit();
      } else {
        throw new Exception("Final time is out of supported range");
      }
    }
  }

  private const int factor = 4;
}

}
