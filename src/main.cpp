/*
 * ----- CLEO -----
 * File: main.cpp
 * Project: src
 * Created Date: Thursday 12th October 2023
 * Author: Clara Bayley (CB)
 * Additional Contributors:
 * -----
 * Last Modified: Tuesday 17th October 2023
 * Modified By: CB
 * -----
 * License: BSD 3-Clause "New" or "Revised" License
 * https://opensource.org/licenses/BSD-3-Clause
 * -----
 * Copyright (c) 2023 MPI-M, Clara Bayley
 * -----
 * File Description:
 * runs the CLEO super-droplet model (SDM)
 * after make/compiling, execute for example via:
 * ./src/runCLEO ../src/config/config.txt
 */

#include <iostream>
#include <stdexcept>
#include <string_view>
#include <concepts>

#include <Kokkos_Core.hpp>

#include "cartesiandomain/cartesianmaps.hpp"
#include "coupldyn_fromfile/fromfiledynamics.hpp"
#include "gridboxes/gridboxmaps.hpp"
#include "initialise/config.hpp"
#include "initialise/timesteps.hpp"
#include "observers/constintervalobs.hpp"
#include "observers/observers.hpp"
#include "runcleo/coupleddynamics.hpp"
#include "runcleo/runcleo.hpp"
#include "runcleo/sdmmethods.hpp"
#include "superdrops/condensation.hpp"
#include "superdrops/microphysicalprocess.hpp"
#include "superdrops/predcorrmotion.hpp"
#include "zarr/fsstore.hpp"

CoupledDynamics auto
create_coupldyn(const Config &config,
                const unsigned int coupldynstep)
{
  return FromFileDynamics(config, coupldynstep); 
}

GridboxMaps auto
create_gbxmaps(const Config &config)
{
  return CartesianMaps(config); 
}

MicrophysicalProcess auto
create_microphysics(const Timesteps &tsteps)
{
  return Condensation(tsteps.get_condstep());
}

Motion auto
create_motion(const unsigned int motionstep)
{
  return PredCorrMotion(motionstep); 
}

Observer auto
create_observer(const unsigned int obsstep)
{
  return ConstIntervalObs(obsstep); 
}

auto create_sdm(const Config &config,
                const Timesteps &tsteps,
                const CoupledDynamics auto &coupldyn)
{
  const GridboxMaps auto gbxmaps(create_gbxmaps(config));
  const MicrophysicalProcess auto microphys(create_microphysics(tsteps));
  const MoveSupersInDomain movesupers(create_motion(tsteps.get_motionstep()));
  const Observer auto obs(create_observer(tsteps.get_obsstep()));

  return SDMMethods(coupldyn, gbxmaps,
                    microphys, movesupers, obs);
}

int main(int argc, char *argv[])
{
  if (argc < 2)
  {
    throw std::invalid_argument("configuration file(s) not specified");
  }
 
  Kokkos::Timer kokkostimer;

  /* Read input parameters from configuration file(s) */
  const std::string_view config_filename(argv[1]);    // path to configuration file
  const Config config(config_filename);
  const Timesteps tsteps(config); // timesteps for model (e.g. coupling and end time)

  /* Create zarr store for writing output to storage */
  FSStore fsstore(config.zarrbasedir);
    
  /* Solver of dynamics coupled to CLEO SDM */
  const CoupledDynamics auto coupldyn(
      create_coupldyn(config, tsteps.get_couplstep()));

  /* CLEO Super-Droplet Model (excluding coupled dynamics solver) */
  const SDMMethods sdm(create_sdm(config, tsteps, coupldyn));

  /* Method to create Super-droplets using initial conditions */
  const InitConds init(config);

  /* Run CLEO (SDM coupled to dynamics solver) */
  Kokkos::initialize(argc, argv);
  {
    const RunCLEO runcleo(coupldyn, sdm);
    runcleo(init, tsteps.get_t_end());
  }
  Kokkos::finalize();
  
  const double ttot(kokkostimer.seconds());
  std::cout << "-----\n Total Program Duration: "
            << ttot << "s \n-----\n";

  return 0;
}
