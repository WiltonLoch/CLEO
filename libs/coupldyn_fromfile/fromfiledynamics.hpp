/*
 * ----- CLEO -----
 * File: fromfiledynamics.hpp
 * Project: coupldyn_fromfile
 * Created Date: Friday 13th October 2023
 * Author: Clara Bayley (CB)
 * Additional Contributors:
 * -----
 * Last Modified: Monday 16th October 2023
 * Modified By: CB
 * -----
 * License: BSD 3-Clause "New" or "Revised" License
 * https://opensource.org/licenses/BSD-3-Clause
 * -----
 * Copyright (c) 2023 MPI-M, Clara Bayley
 * -----
 * File Description:
 * struct obeying coupleddyanmics concept for
 * dynamics solver in CLEO where coupling is
 * one-way and dynamics are read from file
 */

#ifndef FROMFILEDYNAMICS_HPP 
#define FROMFILEDYNAMICS_HPP 

#include <iostream>

#include "initialise/config.hpp"

struct FromFileDynamics
{
private:
  const unsigned int interval;

  void run_dynamics(const unsigned int t_mdl,
                    const unsigned int stepsize) const;
  
public:
  FromFileDynamics(const Config &config,
                   const unsigned int couplstep)
      : interval(couplstep) {}

  unsigned int get_couplstep() const
  {
    return interval;
  }

  void prepare_to_timestep() const;

  bool on_step(const unsigned int t_mdl) const
  {
    return t_mdl % interval == 0;
  }

  void run_step(const unsigned int t_mdl,
                const unsigned int stepsize) const
  {
    if (on_step(t_mdl))
    {
      run_dynamics(t_mdl, stepsize);
    }
  }

};

#endif // FROMFILEDYNAMICS_HPP