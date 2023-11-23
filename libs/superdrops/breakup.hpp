/*
 * ----- CLEO -----
 * File: breakup.hpp
 * Project: superdrops
 * Created Date: Friday 13th October 2023
 * Author: Clara Bayley (CB)
 * Additional Contributors:
 * -----
 * Last Modified: Thursday 16th November 2023
 * Modified By: CB
 * -----
 * License: BSD 3-Clause "New" or "Revised" License
 * https://opensource.org/licenses/BSD-3-Clause
 * -----
 * Copyright (c) 2023 MPI-M, Clara Bayley
 * -----
 * File Description:
 * functionality to enact collision-breakup events
 * in SDM analagous to to Shima et al. 2009.
 * Breakup struct satisfies PairEnactX
 * concept used in Collisions struct */

#ifndef BREAKUP_HPP
#define BREAKUP_HPP

#include <functional>

#include <Kokkos_Core.hpp>

#include "./collisions.hpp"
#include "./microphysicalprocess.hpp"
#include "./superdrop.hpp"

struct DoBreakup
{
private:
public:
  KOKKOS_INLINE_FUNCTION
  bool operator()(Superdrop &drop1, Superdrop &drop2,
                  const double prob, const double phi) const;
  /* this operator is used as an "adaptor" for
  using DoBreakup as a function in DoCollisions
  that satistfies the PairEnactX concept */
};

template <PairProbability Probability>
inline MicrophysicalProcess auto
CollBu(const unsigned int interval,
         const std::function<double(unsigned int)> int2realtime,
         const Probability collbuprob)
/* constructs Microphysical Process for collision-breakup
of superdroplets with a constant timestep 'interval' and
probability of collision-breakup determined by 'collbuprob' */
{
  const double DELT(int2realtime(interval));

  const DoBreakup bu{};
  const DoCollisions<Probability, DoBreakup> colls(DELT,
                                                   collbuprob,
                                                   bu);

  return ConstTstepMicrophysics(interval, colls);
}

/* -----  ----- TODO: move functions below to .cpp file ----- ----- */

#endif // BREAKUP_HPP