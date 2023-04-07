// Author: Clara Bayley
// File: run_coupledmodel.cpp
/* Functions to run SDM coupled to
CVODE ODE thermodyanmics solver */

#include "run_coupledmodel.hpp"

// TO DO: use pushback for init_thermodynamics y_init vector creation

std::vector<double> init_thermodynamics(const size_t num_gridboxes,
                                        const Config &config)
/* return vector of dimensionless initial conditions
for thermodyanmic variables (p, temp, qv, qc) to
initialise cvode thermodynamics solver */
{
  constexpr int NVARS = 4;                   // no. (distinct) variables per grid box
  const size_t neq = NVARS * num_gridboxes;     // total no. variables
  std::vector<double> y_init(neq);

  const double p_init = config.P_INIT / dlc::P0;
  const double temp_init = config.TEMP_INIT / dlc::TEMP0;
  const double vapourp_init = saturation_pressure(temp_init) * config.relh_init / 100.0;
  const double qv_init = vapourpressure_2_massmixratio(vapourp_init, p_init);
  const double qc_init = config.qc_init;

  for (size_t k = 0; k < neq; k += NVARS)
  {
    y_init[k] = p_init;
    y_init[k + 1] = temp_init;
    y_init[k + 2] = qv_init;
    y_init[k + 3] = qc_init;
  }

  return y_init;
}

std::mt19937 prepare_coupledmodel(const Timesteps &mdlsteps, CvodeThermoSolver &cvode,
                                  std::vector<GridBox> &gridboxes)
/* print some details about the cvode thermodynamics solver setup and
return a random number generator */
{
  cvode.print_init_ODEdata(timestep2dimlesstime(mdlsteps.outstep),
                           timestep2dimlesstime(mdlsteps.tend));

  for (long unsigned int ii = 0; ii < gridboxes.size(); ++ii)
  {
    set_thermostate(ii, gridboxes[ii].state, cvode);
  }

  set_superdroplets_to_wetradius(gridboxes);

  return std::mt19937(std::random_device()());
}

void set_superdroplets_to_wetradius(std::vector<GridBox> &gridboxes)
/* for each gridbox, set the radius of each superdroplet (SD) to whichever is
larger out of their dry radius or equlibrium wet radius
(given the relative humidity (s_ratio) and temperature of the gridbox).
If relh > maxrelh = 0.95, set each SD's radius to their 
equilibrium radius at relh = maxrelh = 0.95 */
{

  const double maxrelh = 0.95;

  for (auto &gbx : gridboxes)
  {
    const double temp = gbx.state.temp;
    const double psat = saturation_pressure(temp);
    const double s_ratio = std::min(maxrelh, supersaturation_ratio(gbx.state.press,
                                                                   gbx.state.qvap, psat));
    for (auto &SDinGBx : gbx.span4SDsinGBx)
    {
      const double equilwetradius = SDinGBx.superdrop.superdroplet_wet_radius(s_ratio, temp);
      const double dryradius =  SDinGBx.superdrop.get_dry_radius();
      SDinGBx.superdrop.radius = std::max(dryradius, equilwetradius);
    }
  }
}

std::vector<ThermoState> set_thermodynamics_from_cvodesolver(std::vector<GridBox> &gridboxes,
                                                             const CvodeThermoSolver &cvode)
/* get thermo variables from thermodynamics solver and use
these to set ThermoState of each gridbox. Return vector
containing all those Thermostates */
{
  const long unsigned int gridN = gridboxes.size();
  std::vector<ThermoState> currentstates(gridN);

  for (long unsigned int ii = 0; ii < gridN; ++ii)
  {
    set_thermostate(ii, gridboxes[ii].state, cvode);
    currentstates[ii] = gridboxes[ii].state;
  }

  return currentstates;
}

int proceeed_tonext_coupledstep(int t_out, const int outstep,
                                const bool doCouple,
                                const std::vector<ThermoState> &previousstates,
                                std::vector<GridBox> &gridboxes,
                                CvodeThermoSolver &cvode)
/* exchanges superdroplets between gridboxes and sends
changes in thermodynamics due to SDM microphysics to thermodynamics solver
(eg. raise in temperature of a gridbox due to latent heat release) */
{

  if (doCouple)
  {
    thermodynamic_changes_to_cvodesolver(previousstates, gridboxes, cvode);
  }

  return t_out += outstep;
}

void thermodynamic_changes_to_cvodesolver(const std::vector<ThermoState> &previousstates,
                                          const std::vector<GridBox> &gridboxes,
                                          CvodeThermoSolver &cvode)
/* calculate changes in thermodynamics (temp, qv and qc) due to SDM process
affecting ThermoState, then reinitialise cvode solver with those changes */
{
  constexpr int NVARS = 4;
  const long unsigned int gridN = gridboxes.size();

  std::vector<double> delta_y(gridN * NVARS, 0.0);

  for (long unsigned int ii = 0; ii < gridN; ++ii)
  {
    ThermoState delta_state = gridboxes[ii].state - previousstates[ii];

    delta_y[NVARS * ii + 1] = delta_state.temp;
    delta_y[NVARS * ii + 2] = delta_state.qvap;
    delta_y[NVARS * ii + 3] = delta_state.qcond;
  }

  std::vector<double> nodelta(gridN * NVARS, 0.0);
  if (delta_y != nodelta)
  {
    cvode.reinitialise(cvode.get_time(), delta_y);
  }
}

void exchange_superdroplets_between_gridboxes(const Maps4GridBoxes &mdlmaps,
                                              std::vector<SuperdropWithGridbox> &SDsInGBxs,
                                              std::vector<GridBox> &gridboxes)
/* move superdroplets between gridboxes by changing their associated
gridboxindex if necessary, then (re)sorting SDsInGBxs vector and
updating spans4SDsInGbx for each gridbox */
{
  change_superdroplets_gridboxindex(mdlmaps, gridboxes); 

  sort_superdrops_via_gridboxindex(SDsInGBxs); // see superdrops_with_gridboxes.cpp

  set_gridboxes_superdropletspan(gridboxes, SDsInGBxs); // see gridbox.cpp
  
  // for (auto gbx: gridboxes)
  // {
  //   gbx.iscorrect_span_for_gbxindex(mdlmaps);
  // }
}

void change_superdroplets_gridboxindex(const Maps4GridBoxes &mdlmaps,
                                       std::vector<GridBox> &gridboxes)

{
  for (auto &gbx : gridboxes)
  {
    for (auto &SDinGBx : gbx.span4SDsinGBx)
    {
      sdgbxindex_to_neighbour(mdlmaps, SDinGBx); // see superdrops_with_gridboxes.cpp
    }
  }
}