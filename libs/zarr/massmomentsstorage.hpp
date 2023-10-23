/*
 * ----- CLEO -----
 * File: massmomentsstorage.hpp
 * Project: zarr
 * Created Date: Sunday 22nd October 2023
 * Author: Clara Bayley (CB)
 * Additional Contributors:
 * -----
 * Last Modified: Monday 23rd October 2023
 * Modified By: CB
 * -----
 * License: BSD 3-Clause "New" or "Revised" License
 * https://opensource.org/licenses/BSD-3-Clause
 * -----
 * Copyright (c) 2023 MPI-M, Clara Bayley
 * -----
 * File Description:
 * Storage similar to twoDstorage for writing
 * variables to a fsstore via buffers and occording
 * to the Zarr storage specification version 2.0,
 * but extended to more than one variable and with
 * metadata written specifically for the 0th, 1st
 * and 2nd moments of the (real) droplet mass
 * distribution
 */

#ifndef MASSMOMENTSSTORAGE_HPP
#define MASSMOMENTSSTORAGE_HPP

#include <limits>
#include <vector>
#include <string>
#include <tuple>
#include <cassert>
#include <stdexcept>

#include "./fsstore.hpp"
#include "./storehelpers.hpp"
#include "../cleoconstants.hpp"

template <typename T>
struct MassMomentsStorage
/* 2D storage with dimensions [time, gbxindex] for 0th, 1st and 
2nd momments of the (real) droplet mass distirbution.
nobs is number of observation events (no. time outputs)
and ngbxs is the number of elements in 1st dimension
of 2-D data i.e. no. gridboxes observed for each time */
{
private:
  const size_t chunksize;           // fixed size of array chunks (=max no. datapoints in buffer before writing)
  const size_t ngbxs;               // number elements in 1st dimensin (e.g. number of gridboxes that are observed)

  std::string get_name(const std::string mom) const
  {
    return "massmom" + mom + endname;
  }

  void writechunk() const
  /* write data in buffer to a chunk in store alongside metadata jsons */
  {
    const std::string chunknum = std::to_string(this->chunkcount) + ".0";

    storehelpers::writebuffer2chunk(this->store, this->buffers.mom0,
                                    get_name("0"), chunknum,
                                    this->chunkcount);

    storehelpers::writebuffer2chunk(this->store, this->buffers.mom1,
                                    get_name("1"), chunknum,
                                    this->chunkcount);

    std::tie(this->chunkcount, this->buffersfill) = storehelpers::
        writebuffer2chunk(this->store, this->buffers.mom2,
                          get_name("2"), chunknum,
                          this->chunkcount);

    writejsons();
  }

  void writejsons() const
  /* write strictly required metadata to decode chunks (MUST).
  Assert also check 2D data dimensions is as expected */
  {
    assert((this->ndata == nobs * ngbxs) &&
           "1D data length matches 2D array size");
    assert((this->get_chunksize() % ngbxs == 0.0) &&
           "chunks are integer multiple of 1st dimension of 2-D data");

    const auto n1str = std::to_string(ngbxs);
    const auto nobstr = std::to_string(nobs);
    const auto nchstr = std::to_string(this->get_chunksize() / ngbxs);
    const auto shape("[" + nobstr + ", " + n1str + "]");
    const auto chunks("[" + nchstr + ", " + n1str + "]");
    const std::string dims = "[\"time\", \"gbxindex\"]";

    this->massmoments_zarrayjsons(shape, chunks, dims);
  }

protected:
  FSStore &store;            // file system store satisfying zarr store specificaiton v2
  const std::string endname; // name to add to end of massmom[X] being stored
  struct Buffers
  {
    std::vector<T> mom0; // buffer for 0th mass moment data until writing to array chunk
    std::vector<T> mom1; // buffer for 1st mass moment data until writing to array chunk
    std::vector<T> mom2; // buffer for 2nd mass moment data until writing to array chunk

    Buffers(const unsigned int chunksize)
        : mom0(chunksize, std::numeric_limits<T>::max()),
          mom1(chunksize, std::numeric_limits<T>::max()),
          mom2(chunksize, std::numeric_limits<T>::max()) {}
  } buffers;

  unsigned int chunkcount;  // number of chunks of array so far written to store
  unsigned int buffersfill; // number of datapoints so far copied into buffer
  unsigned int ndata;       // number of data points that have been observed

  const char zarr_format = '2';          // storage spec. version 2
  const char order = 'C';                // layout of bytes within each chunk of array in storage, can be 'C' or 'F'
  const std::string compressor = "null"; // compression of data when writing to store
  const std::string fill_value = "null"; // fill value for empty datapoints in array
  const std::string filters = "null";    // codec configurations for compression
  const std::string dtype;               // datatype stored in arrays

  unsigned int get_chunksize() const { return chunksize; }

  void massmoments_zarrayjsons(const std::string shape,
                   const std::string chunks,
                   const std::string dims) const
  /* write array's metadata to .json files */
  {
    const std::string units0 = " ";
    constexpr double scale_factor0 = 1.0;
    zarrayjsons(shape, chunks, dims,
                get_name("0"), units0, scale_factor0);

    const std::string units1 = "g";
    constexpr double scale_factor1 = dlc::MASS0grams; // grams
    zarrayjsons(shape, chunks, dims,
                get_name("1"), units1, scale_factor1);

    const std::string units2 = "g^2";
    constexpr double scale_factor2 = dlc::MASS0grams * dlc::MASS0grams; // grams squared
    zarrayjsons(shape, chunks, dims,
                get_name("2"), units2, scale_factor2);
  }

  void zarrayjsons(const std::string shape,
                   const std::string chunks,
                   const std::string dims,
                   const std::string name,
                   const std::string units,
                   const double scale_factor) const
  /* write array's metadata to .json files */
  {
    const std::string metadata = storehelpers::
        metadata(zarr_format, order, shape, chunks, dtype,
                 compressor, fill_value, filters);

    const std::string arrayattrs = storehelpers::
        arrayattrs(dims, units, scale_factor);

    storehelpers::writezarrjsons(store, name, metadata, arrayattrs);
  }

  void copy2buffer(const T mom0, const T mom1, const T mom2) const
  /* copy value 'val' to buffer */
  {
    storehelpers::val2buffer<T>(mom0, buffers.mom0, buffersfill);
    storehelpers::val2buffer<T>(mom1, buffers.mom1, buffersfill);
    buffersfill = storehelpers::val2buffer<T>(mom2, buffers.mom2, buffersfill);
    ++ndata;
  }

public:
  unsigned int nobs; // number of output times that have been observed

  MassMomentsStorage(FSStore &store, const unsigned int maxchunk,
                     const std::string dtype, const size_t ngbxs,
                     const std::string endname)
      : chunksize(storehelpers::good2Dchunk(maxchunk, ngbxs)),
        ngbxs(ngbxs), store(store), endname(endname),
        buffers(chunksize), chunkcount(0),
        buffersfill(0), ndata(0), dtype(dtype) {}

  ~MassMomentsStorage()
  /* upon destruction write any data leftover in buffer
  to a chunk and write array's metadata to a .json file */
  {
    if (this->buffersfill != 0)
    {
      writechunk();
    }
  }

  int get_ndata() const { return ndata; }

  void massmoments_to_storage(const T mom0, const T mom1, const T mom2) const
  /* write val in the zarr store. First copy it to a buffer,
  then write buffer to a chunk in the store when the number
  of values in the buffer reaches the chunksize */
  {
    if (buffersfill == chunksize)
    {
      writechunk();
    }

    copy2buffer(mom0, mom1, mom2);
  }

  void is_dim1(const size_t goodndim1,
               const std::string &goodname) const
  {
    if (ngbxs != goodndim1)
    {
      const std::string errmsg("ndim1 is" +
                               std::to_string(ngbxs) +
                               ", but should be " +
                               std::to_string(goodndim1));
      throw std::invalid_argument(errmsg);
    }

    const std::string dim1name("gbxindex");
    if (dim1name!= goodname)
    {
      const std::string errmsg("name of dim1 is " + dim1name +
                               ", but should be " + goodname);
      throw std::invalid_argument(errmsg);
    }
  }
};

#endif // MASSMOMENTSSTORAGE_HPP  