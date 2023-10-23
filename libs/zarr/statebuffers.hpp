/*
 * ----- CLEO -----
 * File: statebuffers.hpp
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
 * metadata written specifically for variables
 * in the state of a gridbox
 */

#ifndef STATEBUFFERS_HPP 
#define STATEBUFFERS_HPP 

#include <string>
#include <vector>
#include <limits>
#include <array>
#include <tuple>
#include <utility>

#include "./fsstore.hpp"
#include "./storehelpers.hpp"
#include "../cleoconstants.hpp"
#include "superdrops/state.hpp"

template <typename T>
struct StateBuffers
{
private:
  std::vector<T> press;     
  std::vector<T> temp;       
  std::vector<T> qvap;       
  std::vector<T> qcond;       



public:
  StateBuffers(const std::string endname,
               const unsigned int chunksize)
      : press(chunksize, std::numeric_limits<T>::max()),
        temp(chunksize, std::numeric_limits<T>::max()),
        qvap(chunksize, std::numeric_limits<T>::max()),
        qcond(chunksize, std::numeric_limits<T>::max()) {}

  void writejsons(FSStore &store,
                  const std::string &metadata) const
  /* write array's metadata to .json files */
  {
    const std::string dims = "[\"time\", \"gbxindex\"]";

    storehelpers::writezarrjsons(store, "press", metadata,
                                 dims, "hPa", dlc::P0 / 100);

    storehelpers::writezarrjsons(store, "temp", metadata,
                                 dims, "K", dlc::TEMP0);

    storehelpers::writezarrjsons(store, "qvap", metadata,
                                 dims, " ", 1.0);

    storehelpers::writezarrjsons(store, "qcond", metadata,
                                 dims, " ", 1.0);
  }
};


#endif //  STATEBUFFERS_HPP  