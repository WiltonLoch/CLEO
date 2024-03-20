/*
 * Copyright (c) 2024 MPI-M, Clara Bayley
 *
 *
 * ----- CLEO -----
 * File: xarray_zarr_array.hpp
 * Project: zarr
 * Created Date: Monday 18th March 2024
 * Author: Clara Bayley (CB)
 * Additional Contributors:
 * -----
 * Last Modified: Wednesday 20th March 2024
 * Modified By: CB
 * -----
 * License: BSD 3-Clause "New" or "Revised" License
 * https://opensource.org/licenses/BSD-3-Clause
 * -----
 * File Description:
 * Structure to create a group obeying the Zarr storage specification version 2
 * (https://zarr.readthedocs.io/en/stable/spec/v2.html) in a given memory store.
 */

#ifndef ROUGHPAPER_ZARR_XARRAY_ZARR_ARRAY_HPP_
#define ROUGHPAPER_ZARR_XARRAY_ZARR_ARRAY_HPP_

#include <Kokkos_Core.hpp>
#include <Kokkos_Pair.hpp>
#include <unordered_map>

#include "./zarr_array.hpp"

inline std::vector<size_t> reduced_arrayshape_from_dims(
    const std::unordered_map<std::string, size_t>& datasetdims,
    const std::vector<std::string>& dimnames) {
  auto reduced_arrayshape = std::vector<size_t>({});

  for (size_t aa = 1; aa < dimnames.size(); ++aa) {
    const auto it = datasetdims.find(dimnames.at(aa));
    reduced_arrayshape.push_back(it->second);
  }

  return reduced_arrayshape;
}

/** Zarr array with additional metadata and constraining of shape of array to shape of dimensions
 * in order to ensure Zarr array is compatibile with NetCDF and Xarray
 * @param units The units of the array's coordinates.
 * @param arraydims The names of each dimension of the array.
 */
template <typename Store, typename T>
class XarrayZarrArray {
 private:
  // TODO(CB) move aliases to aliases.hpp
  ZarrArray<Store, T> zarr;
  std::unordered_map<std::string, size_t> arraydims;

  void set_arrayshape_from_dims() {}

  /**
   * @brief Writes chunks of data from a kokkos view in host memory to the Zarr array in a store.
   *
   * Calls write_chunks_to_store to write whole chunks of data into store. Then updates the shape of
   * each of the dimensions of the array to be consistent with the accumulated change in shape of
   * the array (due to the chunks that have been written). Note however, this function does not
   * (re-)write the .zarray json file's metadata for the shape of the array.
   * Function returns a (sub)view of the remaining data not written to a chunk (number of elements
   * in subview < chunksize).
   *
   * @param h_data Kokkos view of the data to write to the store in host memory.
   * @return The remaining data that was not written to chunks.
   */
  subviewh_buffer write_chunks_with_xarray_metadata(
      const std::unordered_map<std::string, size_t>& datasetdims, const subviewh_buffer h_data) {
    const auto shape_increment = write_chunks_to_store(h_data);

    if (shape_increment) {
      update_arraydims(datasetdims, store, shape_increment);
    }

    const auto n_to_chunks = nchunks_data * buffer.get_chunksize();
    const auto refs = kkpair_size_t({n_to_chunks, h_data.extent(0)});
    return Kokkos::subview(h_data, refs);
  }

 public:
  XarrayZarrArray(Store& store, const std::unordered_map<std::string, size_t>& datasetdims,
                  const std::string_view name, const std::string_view units,
                  const std::string_view dtype, const double scale_factor,
                  const std::vector<size_t>& chunkshape, const std::vector<std::string>& dimnames)
      : ZarrArray(store, name, dtype, chunkshape,
                  reduced_arrayshape_from_dims(datasetdims, dimnames)),
        arraydims() {
    assert((chunkshape.size() == dimnames.size()) &&
           "number of named dimensions of array must match number dimensions of chunks");

    /* make string of zattrs attribute information for array in zarr store */
    const auto arrayattrs = std::string(
        "{\n"
        "  \"_ARRAY_DIMENSIONS\": " +
        vecstr_to_string(dimnames) +  // names of each dimension of array
        ",\n"
        "  \"units\": " +
        "\"" + std::string(units) + "\"" +  // units of coordinate being stored
        ",\n"
        "  \"scale_factor\": " +
        std::to_string(scale_factor) +  // scale_factor of data
        "\n}");

    write_zattrs_json(store, name, arrayattrs);

    for (size_t aa = 0; aa < dimnames.size(); ++aa) {
      const auto it = datasetdims.find(dimnames.at(aa));
      arraydims.insert(*it);
    }
  }

  /**
   * @brief Writes data from Kokkos view in host memory to chunks of a Zarr array in a store
   * via a buffer.
   *
   * Copies some data from the view to a buffer (until number of elements in buffer = chunksize),
   * then may write chunks of the array alongside the necessary metadata for a Zarr array into
   * a store. Finall copies any leftover data, number of elements < chunksize, into the buffer.
   * Assertion checks there is no remainng data unattended to.
   *
   * @param h_data The data in a Kokkos view in host memory which should be written to the array in
   * a store.
   */
  std::unordered_map<std::string, size_t> write_to_array(
      const std::unordered_map<std::string, size_t>& datasetdims, const viewh_buffer h_data) {
    auto h_data_rem = buffer.copy_to_buffer(h_data);

    h_data_rem = write_chunks_with_xarray_metadata(datasetdims, h_data_rem);

    h_data_rem = buffer.copy_to_buffer(h_data_rem);

    /* TODO(CB) docstrings */
    /* TODO(CB) update .zarray json file for Zarr metadata about shape of array according to
     * shape of dimensions as required for xarray and NetCDF compatibility -> eventually call
     something like "set_arrayshape(shape) and write_zarray_json(store, name, zarr_metadata());" */

    assert((h_data_rem.extent(0) == 0) && "there is leftover data remaining after writing array");

    return arraydims;
  };
};

#endif  // ROUGHPAPER_ZARR_XARRAY_ZARR_ARRAY_HPP_
