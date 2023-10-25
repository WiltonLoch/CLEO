/*
 * ----- CLEO -----
 * File: kokkosaliases_sd.hpp
 * Project: superdrops
 * Created Date: Saturday 14th October 2023
 * Author: Clara Bayley (CB)
 * Additional Contributors:
 * -----
 * Last Modified: Wednesday 25th October 2023
 * Modified By: CB
 * -----
 * License: BSD 3-Clause "New" or "Revised" License
 * https://opensource.org/licenses/BSD-3-Clause
 * -----
 * Copyright (c) 2023 MPI-M, Clara Bayley
 * -----
 * File Description:
 * aliases for Kokkos superdrop views
 */

#ifndef KOKKOSALIASES_SD_HPP
#define KOKKOSALIASES_SD_HPP

#include <Kokkos_Core.hpp>
#include <Kokkos_DualView.hpp>

#include "./superdrop.hpp"

using ExecSpace = Kokkos::DefaultExecutionSpace;

using viewd_supers = Kokkos::View<Superdrop *>;            // view in device memory of superdroplets (should match that in gridbox.hpp) 
using viewd_constsupers = Kokkos::View<const Superdrop *>; // view in device memory of const superdroplets (should match that in gridbox.hpp)

using subviewd_supers = Kokkos::Subview<viewd_supers, Kokkos::pair<size_t, size_t>>; // subiew should match that in gridbox.hpp
using subviewd_constsupers = Kokkos::Subview<viewd_constsupers, Kokkos::pair<size_t, size_t>>; // subview should match that in gridbox.hpp

using mirrorh_constsupers = subviewd_constsupers::HostMirror; // mirror view (copy) of subview of superdroplets on host memory (should match that in gridbox.hpp)

#endif // KOKKOSALIASES_SD_HPP