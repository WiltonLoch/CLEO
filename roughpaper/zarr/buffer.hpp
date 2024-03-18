/*
 * Copyright (c) 2024 MPI-M, Clara Bayley
 *
 *
 * ----- CLEO -----
 * File: buffer.hpp
 * Project: zarr
 * Created Date: Monday 18th March 2024
 * Author: Clara Bayley (CB)
 * Additional Contributors:
 * -----
 * Last Modified: Monday 18th March 2024
 * Modified By: CB
 * -----
 * License: BSD 3-Clause "New" or "Revised" License
 * https://opensource.org/licenses/BSD-3-Clause
 * -----
 * File Description:
 * Class for a buffer used by a ZarrArray to acculuate data and then write it into a store
 */


#ifndef ROUGHPAPER_ZARR_BUFFER_HPP_
#define ROUGHPAPER_ZARR_BUFFER_HPP_

#include <limits>
#include <vector>
#include <algorithm>
#include <string>
#include <string_view>

/* returns product of a vector of size_t numbers */
inline size_t vec_product(const std::vector<size_t>& vec) {
  auto value = size_t{1};
  for (const auto& v : vec) {
    value *= v;
  }
  return value;
}

/* returns product of a vector of size_t numbers starting from aa'th index of vector */
inline size_t vec_product(const std::vector<size_t>& vec, const size_t aa) {
  auto value = size_t{1};
  for (auto it = vec.begin() + aa; it != vec.end(); ++it) {
    value *= *it;
  }
  return value;
}

/**
 * @brief A class template for managing a buffer of elements of data type T.
 *
 * This class provides functionality for initializing a buffer, copying elements of data into it
 * and writing the buffer to a store.
 *
 * @tparam The type of the store object used by the buffer.
 * @tparam T The type of elements stored in the buffer.
 */
template <typename T>
struct Buffer {
 public:
  // TODO(CB) move aliases to aliases.hpp
  using viewh_buffer = Kokkos::View<T*, HostSpace::memory_space>;   /// View of buffer type on host
  using subviewh_buffer = Kokkos::Subview<viewh_buffer, kkpair_size_t>;   ///< Subview of host view

 private:
  size_t chunksize;                        ///< Total chunk size = product of shape of chunks
  size_t fill;                             ///< Number of elements of buffer currently filled
  viewh_buffer buffer;                     ///< View for buffer in host memory

  /**
   * @brief Parallel loop on host to fill buffer with NaN (numerical limit).
   */
  void reset_buffer() {
    Kokkos::parallel_for(
      "init_buffer", Kokkos::RangePolicy<HostSpace>(0, chunksize),
      KOKKOS_CLASS_LAMBDA(const size_t & jj) {
      buffer(jj) = std::numeric_limits<T>::max();
    });
    fill = 0;
  }

  /**
   * @brief Parallel loop on host to fill buffer with data elements
   *
   * Parallel loop on host to fill buffer from index "fill" (i.e. start of empty spaces)
   * with "n_to_copy" elements from view of data.
   *
   * @param n_to_copy maximum number of elements to copy to the buffer.
   * @param h_data View containing the data to copy.
   */
  void copy_ndata_to_buffer(const size_t n_to_copy, const viewh_buffer h_data) {
    Kokkos::parallel_for(
      "copy_ndata_to_buffer", Kokkos::RangePolicy<HostSpace>(fill, fill + n_to_copy),
      KOKKOS_CLASS_LAMBDA(const size_t & jj) {
      buffer(jj) = h_data(jj);
    });
    fill += n_to_copy;
  }

 public:
  /**
   * @brief Constructor for the Buffer class.
   *
   * Initializes the buffer with size of given chunkshape.
   *
   * @param chunkshape Vector representing the shape of array chunks.
   */
  explicit Buffer(const std::vector<size_t>& chunkshape) : chunksize(vec_product(chunkshape)),
    fill(0), buffer("buffer", chunksize) {
    reset_buffer();
  }

  /**
   * @brief Gets the total chunk size of the buffer.
   *
   * @return The total chunk size.
   */
  size_t get_chunksize() {
    return chunksize;
  }

  /**
   * @brief Gets the number of elements currently in the buffer.
   *
   * @return The number of elements of buffer filled.
   */
  size_t get_fill() {
    return fill;
  }

  /**
   * @brief Returns the number of empty spaces in the buffer.
   *
   * @return The number of spaces in the buffer currently not filled with data.
   */
  size_t get_space() {
    return chunksize - fill;
  }

  /**
   * @brief Copies as many elements as possible from data to buffer.
   *
   * Copies "n_to_copy" elements from view of data such that either all the data is copied
   * to the buffer or all the spaces in the buffer are filled. Returns a view of remaining
   * data not copied to the buffer which is empty if all the data has been copied.
   *
   * @param h_data View containing the data to copy.
   * @return Subview containing the remaining data not copied to the buffer.
   */
  subviewh_buffer copy_to_buffer(const viewh_buffer h_data) {
    const auto n_to_copy = size_t{ std::min(get_space(), h_data.extent(0)) };

    copy_ndata_to_buffer(n_to_copy, h_data);

    const auto refs = kkpair_size_t({ n_to_copy, h_data.extent(0) });   // indexes of remaining data
    return Kokkos::subview(h_data, refs);
  }

  /**
   * @brief Writes data from buffer to a chunk in a store.
   *
   * Writes data from buffer to a chunk specified by "chunk_label" of an array
   * called "name" in a memory store. Then resets the buffer.
   *
   * @tparam Store The type of the memory store.
   * @param store Reference to the store object.
   * @param name Name of the array in the store.
   * @param chunk_str Name of the chunk of the array to write in the store.
   */
  template<typename Store>
  void write_buffer_to_chunk(Store& store, std::string_view name, const std::string &chunk_label) {
    store[std::string(name) + '/' + chunk_label].operator=<T>(buffer);
    reset_buffer();
  }
};

#endif   // ROUGHPAPER_ZARR_BUFFER_HPP_
