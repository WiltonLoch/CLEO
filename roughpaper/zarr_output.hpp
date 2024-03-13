/*
 * Copyright (c) 2024 MPI-M, Clara Bayley
 *
 *
 * ----- CLEO -----
 * File: zarr_output.hpp
 * Project: roughpaper
 * Created Date: Tuesday 12th March 2024
 * Author: Clara Bayley (CB)
 * Additional Contributors:
 * -----
 * Last Modified: Wednesday 13th March 2024
 * Modified By: CB
 * -----
 * License: BSD 3-Clause "New" or "Revised" License
 * https://opensource.org/licenses/BSD-3-Clause
 * -----
 * File Description:
 */

#ifndef ROUGHPAPER_ZARR_OUTPUT_HPP_
#define ROUGHPAPER_ZARR_OUTPUT_HPP_

#include <iostream>
#include <limits>
#include <algorithm>
#include <cassert>

#include <Kokkos_Core.hpp>
#include <Kokkos_Pair.hpp>
#include <Kokkos_DualView.hpp>

using dualview_type = Kokkos::DualView<double *>;             // dual view of doubles

using kkpair_size_t = Kokkos::pair<size_t, size_t>;
using subview_type = Kokkos::Subview<dualview_type::t_host, kkpair_size_t>;  // subview of host view

using HostSpace = Kokkos::DefaultHostExecutionSpace;
using viewh_buffer = Kokkos::View<double *, HostSpace::memory_space>;   // view for buffer on host

struct Buffer{
 public:
  size_t chunksize;

  explicit Buffer(const size_t i_chunksize) : chunksize(i_chunksize), fill(0),
                                              buffer("buffer", chunksize) {
    reset_buffer();
  }

  /* returns number of spaces in buffer currently not filled with data */
  size_t get_space() {
    return chunksize - fill;
  }

  /* copies as many as possible elements of data to buffer until either all the data is written to
  the buffer, or all the spaces in the buffer are filled. Returns view of remaining data not copied
  to buffer (empty if all the data is copied). */
  subview_type copy_to_buffer(const dualview_type::t_host h_data) {
    // number of elements of data to copy to buffer
    const auto n_to_copy = size_t{std::min(get_space(), h_data.extent(0))};

    // copy "n_to_copy" number of elements of data to buffer
    copy_ndata_to_buffer(n_to_copy, h_data);

    // return remainder of data not copied to buffer
    const auto refs = kkpair_size_t({n_to_copy, h_data.extent(0)});
    return Kokkos::subview(h_data, refs);
  }

  void write_chunk() {
    std::cout << "TODO(CB) write buffer to chunk\n";
    reset_buffer();
  }

 private:
  size_t fill;
  viewh_buffer buffer;

  /* parallel loop on host to fill buffer with nan (numerical limit) values */
  void reset_buffer() {
    Kokkos::parallel_for(
        "init_buffer", Kokkos::RangePolicy<HostSpace>(0, chunksize),
        KOKKOS_CLASS_LAMBDA(const size_t &jj) {
          buffer(jj) = std::numeric_limits<double>::max();
        });
    fill = 0;
  }

  /* parallel loop on host to fill buffer from start of empty spaces (i.e. from index "fill")
  with "n_to_copy" elements from data */
  void copy_ndata_to_buffer(const size_t n_to_copy, const dualview_type::t_host h_data) {
    Kokkos::parallel_for(
        "copy_ndata_to_buffer", Kokkos::RangePolicy<HostSpace>(fill, fill + n_to_copy),
        KOKKOS_CLASS_LAMBDA(const size_t &jj) {
          buffer(jj) = h_data(jj);
        });
    fill = fill + n_to_copy;
  }
};

class ZarrArrayViaBuffer {
 public:
  Buffer buffer;

  explicit ZarrArrayViaBuffer(const size_t i_chunksize) : buffer(i_chunksize){};

  ~ZarrArrayViaBuffer() {
    // write buffer to chunk if it's full
    if (buffer.get_space() < buffer.chunksize) {
      buffer.write_chunk();
    }
  };

  subview_type write_chunks(const subview_type h_data) {
    // write buffer to chunk if it's full
    if (buffer.get_space() == 0) {
      buffer.write_chunk();
    }

    // write whole chunks of h_data_remaining
    const auto nchunks_data = size_t{h_data.extent(0) / buffer.chunksize};
    std::cout << "nchunks from h_data: " << nchunks_data << "\n";
    for (size_t jj = 0; jj < nchunks_data; ++jj) {
      std::cout << "writing chunk directly from h_data no: " << jj << "\n";
    }

    // return remainder of data not written to chunks
    const auto n_to_chunks = nchunks_data * buffer.chunksize;
    const auto refs = kkpair_size_t({n_to_chunks, h_data.extent(0)});
    return Kokkos::subview(h_data, refs);
  }

  void write_array(const dualview_type::t_host h_data) {
    std::cout << "writing data to buffer / output\n";

    std::cout << "buffer size: " << buffer.chunksize << "\n";
    std::cout << "buffer space: " << buffer.get_space() << "\n";
    std::cout << "initial data to add: " << h_data.extent(0) << "\n";

    auto h_data_rem = buffer.copy_to_buffer(h_data);

    std::cout << "after copy to buffer: " << h_data_rem.extent(0) << "\n";
    std::cout << "buffer space: " << buffer.get_space() << "\n";

    h_data_rem = write_chunks(h_data_rem);

    std::cout << "after writing to chunks: " << h_data_rem.extent(0) << "\n";
    std::cout << "buffer space: " << buffer.get_space() << "\n";

    h_data_rem = buffer.copy_to_buffer(h_data_rem);

    assert((h_data_rem.extent(0) == 0) && "there is leftover data remaining after writing array");

    std::cout << "final remaining data: " << h_data_rem.extent(0)<< "\n";
    std::cout << "buffer space: " << buffer.get_space() << "\n";
  };
};

#endif    // ROUGHPAPER_ZARR_OUTPUT_HPP_
