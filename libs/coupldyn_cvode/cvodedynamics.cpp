/*
 * ----- CLEO -----
 * File: cvodedynamics.cpp
 * Project: coupldyn_cvode
 * Created Date: Friday 13th October 2023
 * Author: Clara Bayley (CB)
 * Additional Contributors:
 * -----
 * Last Modified: Friday 27th October 2023
 * Modified By: CB
 * -----
 * License: BSD 3-Clause "New" or "Revised" License
 * https://opensource.org/licenses/BSD-3-Clause
 * -----
 * Copyright (c) 2023 MPI-M, Clara Bayley
 * -----
 * File Description:
 * functionality for coupleddyanmics concept for
 * dynamics solver in CLEO where coupling is
 * two-way to cvode adiabatic parcel ODE solver
 */

#include "./cvodedynamics.hpp"

double saturation_pressure(const double temp)
/* Calculate the equilibrium vapor pressure of water over liquid
water ie. the saturation pressure (psat). Equation taken from
Bjorn Steven's "make_tetens" python function from his module
"moist_thermodynamics.saturation_vapour_pressures" available
on gitlab. Original paper "Murray, F. W. On the Computation of
Saturation Vapor Pressure. Journal of Applied Meteorology
and Climatology 6, 203–204 (1967)." Note function is called
with conversion to real temp /K = T*Temp0 and from real psat
to dimensionless psat = psat/P0. */
{
  assert((temp > 0) && "psat ERROR: temperature must be larger than 0K.");

  constexpr double A = 17.4146; // constants from Bjorn Gitlab originally from paper
  constexpr double B = 33.639; // ditto
  constexpr double TREF = 273.16;  // Triple point temperature [K] of water
  constexpr double PREF = 611.655; // Triple point pressure [Pa] of water

  const double T(temp * dlc::TEMP0); // real T [K]

  return (PREF * std::exp(A * (T - TREF) / (T - B))) / dlc::P0; // dimensionless psat
}

double mass_mixing_ratio(const double press_vapour,
                    const double press)
/* Calculate mass mixing ratio, qv = m_v/m_dry = rho_v/rho_dry
given the vapour pressure, pv = p_v/p_tot and total pressure p_tot */
{
  return dlc::Mr_ratio * press_vapour / (press - press_vapour);
}

void CvodeDynamics::prepare_to_timestep() const
{
  print_init_ODEdata(step2dimlesstime(couplstep),
                     step2dimlesstime(t_end));
}


void CvodeDynamics::run_dynamics(const unsigned int t_mdl,
                                    const unsigned int t_next) const
{
  previousstates = //TODO
  
  cvode.run_cvodestep(t_mdl, couplstep,
                        step2dimlesstime(t_mdl + couplstep));
}

CvodeDynamics::CvodeDynamics(const Config &config,
                             const unsigned int couplstep)
    /* construct instance of CVODE ODE solver with initial conditions */
    : interval(couplstep),
      A(NULL),
      LS(NULL),
      cvode_mem(NULL),
      retval(0),
      neq(NVARS*config.ngbxs),
      t(0.0),
      y(NULL),
      re_y(NULL),
      ATOLS(NULL)
{
  data = (UserData)malloc(sizeof *data);
  previousstates = initial_cvode_conditions(config);

  const double wmax = (M_PI / 2) * (config.W_AVG / dlc::W0);  // dimensionless w velocity passed to thermo ODEs eg. dp_dt(t,y,ydot,w,...)
  const double tauhalf = (config.T_HALF / dlc::TIME0) / M_PI; // dimensionless timescale for w sinusoid
  init_userdata(neq, config.doThermo, wmax, tauhalf);
  setup_ODE_solver(config.cvode_rtol, config.cvode_atol);
}

~CvodeDynamics::CvodeDynamics()
{
  /* print final statistics to the terminal screen */
  std::cout << "\nLast Iteration Statistics:\n";
  retval = CVodePrintAllStats(cvode_mem, stdout, SUN_OUTPUTFORMAT_TABLE);

  /* free memory */
  N_VDestroy(y);            /* Free y vector */
  N_VDestroy(ATOLS);        /* Free abstol vector */
  free(data);               /* free user_data pointer struc */
  CVodeFree(&cvode_mem);    /* Free CVODE memory */
  SUNLinSolFree(LS);        /* Free the linear solver memory */
  SUNMatDestroy(A);         /* Free the matrix memory */
  SUNContext_Free(&sunctx); /* Free the SUNDIALS context */
}

std::vector<double>
CvodeDynamics::initial_conditions(const Config &config) const
/* return vector of dimensionless initial conditions
for thermodyanmic variables (p, temp, qv, qc) to
initialise cvode thermodynamics solver */
{
  const double press_i(config.P_INIT / dlc::P0);
  const double temp_i(config.TEMP_INIT / dlc::TEMP0);
  const double qcond_i(config.qc_init);

  const double psat(saturation_pressure(temp_i));
  const double vapp(psat * config.relh_init / 100.0); // initial vapour pressure
  const double qvap_i(mass_mixing_ratio(vapp, press_i));

  std::vector<double> y_init(neq);
  for (size_t k = 0; k < neq; k += NVARS)
  {
    y_init.at(k) = press_i;
    y_init.at(k + 1) = temp_i;
    y_init.at(k + 2) = qvap_i;
    y_init.at(k + 3) = qcond_i;
  }

  return y_init;
}

void CvodeDynamics::init_userdata(const size_t neq,
                                      const bool doThermo,
                                      const double wmax,
                                      const double tauhalf)
/* set values in UserData structure for odes_func */
{
  data->neq = neq;
  data->doThermo = doThermo;
  data->wmax = wmax;
  data->tauhalf = tauhalf;
};

int CvodeDynamics::setup_ODE_solver(const double i_rtol,
                                    const double i_atol)
/* function does all the setup steps in order
to use CVODE sundials ODE solver */
{
  /* 0. Create the SUNDIALS context */
  retval = SUNContext_Create(NULL, &sunctx);
  if (check_retval(&retval, "SUNContext_Create", 1))
  {
    retval = 1;
  }

  /*  1. Initialize parallel or multi-threaded environment */
  // ------------------- (optional) --------------------- //

  /* 2. Set the scalar relative and vector absolute tolerances */
  RTOL = i_rtol;
  ATOLS = N_VNew_Serial(neq, sunctx);
  if (check_retval((void *)ATOLS, "N_VNew_Serial", 0))
    return (1);

  for (size_t i = 0; i < neq; ++i)
  {
    NV_Ith_S(ATOLS, i) = i_atol
  }

  /* 3. initialise y vector with initial conditions */
  y = N_VNew_Serial(neq, sunctx);
  if (check_retval((void *)y, "N_VNew_Serial", 0))
    return (1);
  for (size_t i = 0; i < neq; ++i)
  {
    NV_Ith_S(y, i) = previousstates.at(i);
  }

  /* 4. Call CVodeCreate to create the solver memory and specify the
   * Backward Differentiation Formula (CV_BDF) */
  cvode_mem = CVodeCreate(CV_BDF, sunctx);
  if (check_retval((void *)cvode_mem, "CVodeCreate", 0))
    return (1);

  /* 5. Call CVodeInit to initialize the integrator memory and specify the
   * user's right hand side function in y'=f(t,y), the initial time T0=0.0,
   * and the initial dependent variable vector y. */
  retval = CVodeInit(cvode_mem, odes_func, 0.0, y);
  if (check_retval(&retval, "CVodeInit", 1))
    return (1);

  /* 6. Set linear solver optional inputs.
   * Provide user data which can be accessed in user provided routines */
  retval = CVodeSetUserData(cvode_mem, data);
  if (check_retval((void *)&retval, "CVodeSetUserData", 1))
    return (1);

  /* 7. Call CVodeSVtolerances to specify the scalar relative tolerance
   * and vector absolute tolerances */
  retval = CVodeSVtolerances(cvode_mem, RTOL, ATOLS);
  if (check_retval(&retval, "CVodeSVtolerances", 1))
    return (1);

  /* 8. Create dense SUNMatrix for use in linear solves */
  A = SUNDenseMatrix(neq, neq, sunctx);
  if (check_retval((void *)A, "SUNDenseMatrix", 0))
    return (1);

  /* 9. Create dense SUNLinearSolver object for use by CVode */
  LS = SUNLinSol_Dense(y, A, sunctx);
  if (check_retval((void *)LS, "SUNLinSol_Dense", 0))
    return (1);

  /* 10. Attach the matrix and linear solver to CVODE */
  retval = CVodeSetLinearSolver(cvode_mem, LS, A);
  if (check_retval(&retval, "CVodeSetLinearSolver", 1))
    return (1);

  return 0;
};

int CvodeDynamics::check_retval(void *returnvalue,
                                const char *funcname,
                                int opt)
/* Check function return value...
  opt == 0 means SUNDIALS function allocates memory so check if
           returned NULL pointer
  opt == 1 means SUNDIALS function returns an integer value so check if
           retval < 0
  opt == 2 means function allocates memory so check if returned
           NULL pointer
*/
{
  int *retval;

  /* Check if SUNDIALS function returned NULL pointer - no memory allocated */
  if (opt == 0 && returnvalue == NULL)
  {
    std::cout << stderr << "\nCVODE_SUNDIALS_ERROR: %s() failed - returned NULL pointer\n\n"
              << funcname << " \n";
    return (1);
  }

  /* Check if retval < 0 */
  else if (opt == 1)
  {
    retval = (int *)returnvalue;
    if (*retval < 0)
    {
      std::cout << stderr << "\nCVODE_SUNDIALS_ERROR: %s() failed with retval = %d\n\n"
                << funcname << " " << *retval << '\n';
      return (1);
    }
  }

  /* Check if function returned NULL pointer - no memory allocated */
  else if (opt == 2 && returnvalue == NULL)
  {
    std::cout << stderr << "\nMEMORY_ERROR: %s() failed - returned NULL pointer\n\n"
              << funcname << '\n';
    return (1);
  }

  return (0);
}

