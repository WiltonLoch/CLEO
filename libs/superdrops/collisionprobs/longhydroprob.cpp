/*
 * Copyright (c) 2024 MPI-M, Clara Bayley
 *
 *
 * ----- CLEO -----
 * File: longhydroprob.cpp
 * Project: collisionprobs
 * Created Date: Thursday 9th November 2023
 * Author: Clara Bayley (CB)
 * Additional Contributors:
 * -----
 * Last Modified: Monday 11th March 2024
 * Modified By: CB
 * -----
 * License: BSD 3-Clause "New" or "Revised" License
 * https://opensource.org/licenses/BSD-3-Clause
 * -----
 * File Description:
 * functionality for probability of collision-coalescence event between two (real) droplets using
 * the hydrodynamic (i.e. gravitational) kernel according to Simmel et al. 2002's formulation of
 * Long's Hydrodynamic Kernel. Probability calculations are contained in structures that satisfy
 * the requirements of the PairProbability concept (see collisions.hpp)
 */


#include "./longhydroprob.hpp"

/* returns the efficiency of collision-coalescence, eff, according to
  equations 12 and 13 of Simmel et al. 2002). eff = eff(R,r) where R>r.
  eff = colleff(R,r) * coaleff(R,r). Usually it's assumed that
  coaleff(R,r) = 1, ie. eff = colleff, which also means that for
  collisions where R > rlim, eff(R,r) = colleff(R,r) = 1). */
KOKKOS_FUNCTION
double LongHydroProb::kerneleff(const Superdrop &drop1, const Superdrop &drop2) const {
  constexpr double rlim =
      5e-5 / dlc::R0;  // 50 micron limit to determine collision-coalescence efficiency (eff)
  constexpr double colleff_lim = 0.001;  // minimum efficiency if larger droplet's radius < rlim
  constexpr double A1 =
      4.5e4 * dlc::R0 * dlc::R0;  // constants in efficiency calc if larger droplet's radius < rlim
  constexpr double A2 = 3e-4 / dlc::R0;

  const auto smallr = double{Kokkos::fmin(drop1.get_radius(), drop2.get_radius())};
  const auto bigr = double{Kokkos::fmax(drop1.get_radius(), drop2.get_radius())};

  /* calculate collision-coalescence efficiency, eff = colleff * coaleff */
  auto colleff = double{1.0};
  if (bigr < rlim) {
    const auto colleff_calc = double{A1 * bigr * bigr * (1 - A2 / smallr)};
    colleff = Kokkos::fmax(colleff_calc, colleff_lim);  // colleff >= colleff_lim
  }

  const auto eff = colleff * coaleff;

  return eff;
}
