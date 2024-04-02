/*
 * Copyright (c) 2024 MPI-M, Clara Bayley
 *
 *
 * ----- CLEO -----
 * File: do_write_superdrops.hpp
 * Project: observers2
 * Created Date: Wednesday 24th January 2024
 * Author: Clara Bayley (CB)
 * Additional Contributors:
 * -----
 * Last Modified: Tuesday 2nd April 2024
 * Modified By: CB
 * -----
 * License: BSD 3-Clause "New" or "Revised" License
 * https://opensource.org/licenses/BSD-3-Clause
 * -----
 * File Description:
 * Template for an struct which writes data collected from super-droplets in parallel
 * to individual ragged arrays in a dataset
 */

#ifndef LIBS_OBSERVERS2_DO_WRITE_SUPERDROPS_HPP_
#define LIBS_OBSERVERS2_DO_WRITE_SUPERDROPS_HPP_

#include <Kokkos_Core.hpp>
#include <concepts>
#include <iostream>
#include <memory>

#include "../kokkosaliases.hpp"
#include "./write_gridbox_to_array.hpp"
#include "gridboxes/gridbox.hpp"
#include "zarr2/dataset.hpp"
#include "zarr2/xarray_zarr_array.hpp"

/* template class for observer with at_start_step function that collects variables from all the
super-droplets in each gridbox in parallel and then writes them to their respective ragged arrays in
a dataset */
template <typename Store, typename ParallelLoopPolicy,
          WriteGridboxToArray<Store> WriteSuperdropsToArray>
class DoWriteSuperdrops {
 private:
  Dataset<Store> &dataset;  ///< dataset to write data to
  std::shared_ptr<XarrayZarrArray<Store, uint32_t>>
      raggedcount_xzarr_ptr;  ///< pointer to ragged count array in dataset
  WriteSuperdropsToArray
      write2array;  ///< object collects data from gridboxes and writes it to arrays in the dataset
  ParallelLoopPolicy parallel_loop;  ///< function like object to call during at_start_step to
                                     ///< loop over gridboxes

  /* Use the writer's functor to collect data from gridboxes in parallel.
  Then write the datat to arrays in the dataset */
  void at_start_step(const viewd_constgbx d_gbxs) const {
    auto functor = write2array.get_functor(d_gbxs);
    parallel_loop(functor, d_gbxs);
    write2array.write_to_array(dataset);
    dataset.raggedcount_xzarr_ptr.write_to_array(nsupers);  // TODO(CB)
  }

 public:
  DoWriteSuperdrops(ParallelLoopPolicy parallel_loop, Dataset<Store> &dataset,
                    WriteGbxToArray writer, const size_t maxchunk, const size_t ngbxs)
      : dataset(dataset),
        raggedcount_xzarr_ptr(
            std::make_shared<XarrayZarrArray<Store, uint32_t>> dataset.template create_array<T>(
                name, units, dtype, scale_factor, good2Dchunkshape(maxchunk, ngbxs),
                {"time", "gbxindex"})),
        writer(writer),
        parallel_loop(parallel_loop) {}

  ~DoWriteSuperdrops() { write2array.write_arrayshape(dataset); }

  void before_timestepping(const viewd_constgbx d_gbxs) const {
    std::cout << "observer includes write gridboxes observer\n";
  }

  void after_timestepping() const {}

  void at_start_step(const unsigned int t_mdl, const viewd_constgbx d_gbxs) const {
    at_start_step(d_gbxs);
  }
};

#endif  // LIBS_OBSERVERS2_DO_WRITE_SUPERDROPS_HPP_
