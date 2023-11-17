/*
 * ----- CLEO -----
 * File: nulldyncomms.hpp
 * Project: coupldyn_null
 * Created Date: Friday 17th November 2023
 * Author: Clara Bayley (CB)
 * Additional Contributors:
 * -----
 * Last Modified: Friday 17th November 2023
 * Modified By: CB
 * -----
 * License: BSD 3-Clause "New" or "Revised" License
 * https://opensource.org/licenses/BSD-3-Clause
 * -----
 * Copyright (c) 2023 MPI-M, Clara Bayley
 * -----
 * File Description:
 * struct obeying coupleddynamics concept
 * for dynamics solver in CLEO where there
 * is no coupling / communication to SDM
 */


#ifndef NULLCOMMS_HPP 
#define NULLCOMMS_HPP 

#include <Kokkos_Core.hpp>

#include "../kokkosaliases.hpp"
#include "./nulldynamics.hpp"

struct NullDynComms
/* empty (no) coupling to/from to CLEO's gridboxes.
Struct obeys coupling comms concept */
{
  template <typename CD = NullDynComms>
  KOKKOS_INLINE_FUNCTION
  void receive_dynamics(const NullDynamics &nulldyn,
                        const viewh_gbx h_gbxs) const {}
  /* receive information from NullDynamics
  solver if null for no coupling to CLEO SDM */

  template <typename CD = NullDynComms>
  KOKKOS_INLINE_FUNCTION
  void send_dynamics(const viewh_constgbx h_gbxs,
                     const NullDynamics &nulldyn) const {}
  /* send information from Gridboxes' states
  to coupldyn is null for NullDynamics */
};

#endif // NULLCOMMS_HPP 