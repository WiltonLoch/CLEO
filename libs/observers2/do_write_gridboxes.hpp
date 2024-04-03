/*
 * Copyright (c) 2024 MPI-M, Clara Bayley
 *
 *
 * ----- CLEO -----
 * File: do_write_gridboxes.hpp
 * Project: observers2
 * Created Date: Wednesday 24th January 2024
 * Author: Clara Bayley (CB)
 * Additional Contributors:
 * -----
 * Last Modified: Wednesday 3rd April 2024
 * Modified By: CB
 * -----
 * License: BSD 3-Clause "New" or "Revised" License
 * https://opensource.org/licenses/BSD-3-Clause
 * -----
 * File Description:
 * Template for an struct which writes data collected from Gridboxes in parallel
 * to individual arrays in a dataset
 */

#ifndef LIBS_OBSERVERS2_DO_WRITE_GRIDBOXES_HPP_
#define LIBS_OBSERVERS2_DO_WRITE_GRIDBOXES_HPP_

#include <Kokkos_Core.hpp>
#include <concepts>
#include <iostream>
#include <memory>

#include "../kokkosaliases.hpp"
#include "./write_gridbox_to_array.hpp"
#include "gridboxes/gridbox.hpp"
#include "zarr2/dataset.hpp"
#include "zarr2/xarray_zarr_array.hpp"

struct ParallelGbxsRangePolicy {
  template <typename Functor>
  void operator()(Functor functor, const viewd_constgbx d_gbxs) const {
    const size_t ngbxs(d_gbxs.extent(0));
    Kokkos::parallel_for("range_policy_collect_gbxs_data", Kokkos::RangePolicy<ExecSpace>(0, ngbxs),
                         functor);
  }
};

struct ParallelGbxsTeamPolicy {
  template <typename Functor>
  void operator()(Functor functor, const viewd_constgbx d_gbxs) const {
    const size_t ngbxs(d_gbxs.extent(0));
    Kokkos::parallel_for("team_policy_collect_gbxs_data", TeamPolicy(ngbxs, Kokkos::AUTO()),
                         functor);
  }
};

/* template class for observer with at_start_step function that collects variables from each
gridbox in parallel and then writes them to their repspective arrays in a dataset */
template <typename ParallelLoopPolicy, typename Store, WriteGridboxToArray<Store> WriteGbxToArray>
class DoWriteGridboxes {
 private:
  ParallelLoopPolicy parallel_loop;  ///< function like object to call during at_start_step to
                                     ///< loop over gridboxes
  const Dataset<Store> &dataset;     ///< dataset to write data to
  WriteGbxToArray
      write2array;  ///< object collects data from gridboxes and writes it to arrays in the dataset

  /* Use the writer's functor to collect data from gridboxes in parallel.
  Then write the datat to arrays in the dataset */
  void at_start_step(const viewd_constgbx d_gbxs) const {
    auto functor = write2array.get_functor(d_gbxs);
    parallel_loop(functor, d_gbxs);
    write2array.write_to_array(dataset);
  }

 public:
  DoWriteGridboxes(ParallelLoopPolicy parallel_loop, const Dataset<Store> &dataset,
                   WriteGbxToArray write2array)
      : parallel_loop(parallel_loop), dataset(dataset), write2array(write2array) {}

  ~DoWriteGridboxes() { write2array.write_arrayshape(dataset); }

  void before_timestepping(const viewd_constgbx d_gbxs) const {
    std::cout << "observer includes write gridboxes observer\n";
  }

  void after_timestepping() const {}

  void at_start_step(const unsigned int t_mdl, const viewd_constgbx d_gbxs,
                     const viewd_constsupers totsupers) const {
    at_start_step(d_gbxs);
  }
};

#endif  // LIBS_OBSERVERS2_DO_WRITE_GRIDBOXES_HPP_
