﻿#pragma once

#include "integrators/explicit_embedded_runge_kutta_nyström_integrator.hpp"

#include <algorithm>
#include <cmath>
#include <ctime>
#include <vector>

#include "glog/logging.h"
#include "quantities/quantities.hpp"

namespace principia {

using quantities::Difference;
using quantities::Quotient;

namespace integrators {

inline ExplicitEmbeddedRungeKuttaNyströmIntegrator::
           ExplicitEmbeddedRungeKuttaNyströmIntegrator(
    std::vector<double> const& c,
    std::vector<std::vector<double>> const& a,
    std::vector<double> const& b_hat,
    std::vector<double> const& b_prime_hat,
    std::vector<double> const& b,
    std::vector<double> const& b_prime)
    : c_(c),
      a_(a),
      b_hat_(b_hat),
      b_prime_hat_(b_prime_hat),
      b_(b),
      b_prime_(b_prime) {}

template<typename Position>
void ExplicitEmbeddedRungeKuttaNyströmIntegrator::Solve(
    RightHandSideComputation<Position> compute_acceleration,
    SystemState<Position, Variation<Position>> const& initial_value,
    Time const& t_final,
    Time const& first_time_step,
    StepSizeController<Position> step_size_controller,
    double const safety_factor,
    not_null<Solution<Position, Variation<Position>>*> const solution) const {
  using Displacement = Difference<Position>;
  using Velocity = Variation<Position>;
  using Acceleration = Variation<Velocity>;

  // Argument checks.
  int const dimension = initial_value.positions.size();
  CHECK_EQ(initial_value.momenta.size(), dimension);
  CHECK_NE(first_time_step, Time());
  bool const forward = first_time_step > Time();
  if (forward) {
    CHECK_LT(initial_value.time.value, t_final);
  } else {
    // Integrating backward.
    CHECK_GT(initial_value.time.value, t_final);
  }

  // Time step.
  Time h = first_time_step;
  // Current time.
  DoublePrecision<Time> t;

  // Position increment (high-order).
  std::vector<Displacement> ∆q_hat(dimension);
  // Velocity increment (high-order).
  std::vector<Velocity> ∆v_hat(dimension);
  // Current position.
  std::vector<DoublePrecision<Position>> q_hat(dimension);
  // Current velocity.
  std::vector<DoublePrecision<Velocity>> v_hat(dimension);

  // Difference between the low- and high-order approximations of the position.
  std::vector<Displacement> q_error_estimate(dimension);
  // Difference between the low- and high-order approximations of the velocity.
  std::vector<Velocity> v_error_estimate(dimension);

  // Current Runge-Kutta-Nyström stage.
  std::vector<Position> q_stage(dimension);
  // Accelerations at each stage.
  // TODO(egg): this is a rectangular container, use something more appropriate.
  std::vector<std::vector<Acceleration>> g(stages_);
  for (auto& g_stage : g) {
    g_stage.resize(dimension);
  }

  double control_factor = 1.0;
  bool at_end = false;

  while (!at_end) {
    do {
      // TODO(egg): This has to be the silliest way to compute an nth root.
      h *= safety_factor * std::pow(control_factor, 1.0 / (lower_order_ + 1));

      // Termination.
      Time const time_to_end = t_final - t.value - t.error;
      if (forward && h > time_to_end ||
          !forward && h < time_to_end) {
        // The chosen step size will overshoot.  Clip it to just reach the end,
        // and terminate if the step is accepted.
        h = time_to_end;
        at_end = true;
      } else {
        at_end = false;
      }

      // Runge-Kutta-Nyström iteration (fills |g|).
      for (int i = 0; i < stages_; ++i) {
        Time const t_stage = t.value + c_[i] * h;
        for (int k = 0; k < dimension; ++k) {
          Acceleration ∑_a_ij_g_j;
          for (int j = 0; j < i; ++j) {
            ∑_a_ij_g_j += a_[i][j] * g[j][k];
          }
          q_stage[k] = q_hat[k].value +
                           h * (c_[i] * v_hat[k].value + h * ∑_a_ij_g_j);
        }
        compute_acceleration(t_stage, q_stage, &g[i]);
      }
      // TODO(egg): handle the FSAL case.

      // Increment computation and step size control.
      for (int k = 0; k < dimension; ++k) {
        Acceleration ∑_b_hat_i_g_i;
        Acceleration ∑_b_i_g_i;
        Acceleration ∑_b_prime_hat_i_g_i;
        Acceleration ∑_b_prime_i_g_i;
        // Low-order position increment.
        Displacement ∆q_k;
        // Low-order velocity increment.
        Velocity ∆v_k;
        // Please keep eight assigments below aligned, they become illegible
        // otherwise.
        for (int i = 0; i < stages_; ++i) {
          ∑_b_hat_i_g_i       += b_hat_[i] * g[i][k];
          ∑_b_i_g_i           += b_[i] * g[i][k];
          ∑_b_prime_hat_i_g_i += b_prime_hat_[i] * g[i][k];
          ∑_b_prime_i_g_i     += b_prime_[i] * g[i][k];
        }
        ∆q_hat[k] = h * (h * (∑_b_hat_i_g_i) + v_hat[k].value);
        ∆q_k      = h * (h * (∑_b_i_g_i) + v_hat[k].value);
        ∆v_hat[k] = h * ∑_b_prime_hat_i_g_i;
        ∆v_k      = h * ∑_b_prime_i_g_i;

        q_error_estimate[k] = ∆q_k - ∆q_hat[k];
        v_error_estimate[k] = ∆v_k - ∆v_hat[k];
      }
      control_factor = step_size_controller(q_error_estimate, v_error_estimate);
    } while (control_factor < 1.0);

    // Increment the solution with the high-order approximation.
    t.Increment(h);
    for(int k = 0; k < dimension; ++k) {
      q_hat[k].Increment(∆q_hat[k]);
      v_hat[k].Increment(∆v_hat[k]);
    }
    solution->push_back({q_hat, v_hat, t});
  }
}

}  // namespace integrators
}  // namespace principia
