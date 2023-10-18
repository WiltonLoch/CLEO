/*
 * ----- CLEO -----
 * File: createsupers.hpp
 * Project: runcleo
 * Created Date: Tuesday 17th October 2023
 * Author: Clara Bayley (CB)
 * Additional Contributors:
 * -----
 * Last Modified: Wednesday 18th October 2023
 * Modified By: CB
 * -----
 * License: BSD 3-Clause "New" or "Revised" License
 * https://opensource.org/licenses/BSD-3-Clause
 * -----
 * Copyright (c) 2023 MPI-M, Clara Bayley
 * -----
 * File Description:
 * file for functions to create a view of
 * superdroplets (on device) from some 
 * initial conditions
 */

#ifndef CREATESUPERS_HPP
#define CREATESUPERS_HPP

#include <memory>

#include <Kokkos_Core.hpp>

#include "../kokkosaliases.hpp"
#include "superdrops/superdrop.hpp"

template <typename FetchInitData>
class CreateSupers
/* functions (and struct holding data) to
create superdroplets given a type "FetchInitData"
that can return vectors for data on some 
superdroplet inital conditions */
{
private:
  const FetchInitData &fisd; // type used to construct InitData for supers

  class GenSuperdrop
  /* struct holds vectors for data for the initial
  conditions of some superdroplets' properties and
  returns superdrops generated from them */
  {
  private:
    std::unique_ptr<Superdrop::IDType::Gen> sdIdGen; // pointer to superdrop id generator
    viewd_solute solutes;                            // solute(s) stored in memory space of viewd_solute
    std::vector<unsigned int> sdgbxindexes;
    std::vector<double> coord3s;
    std::vector<double> coord1s;
    std::vector<double> coord2s;
    std::vector<double> radii;
    std::vector<double> msols;
    std::vector<unsigned long long> xis;

    SuperdropAttrs attrs_at(const unsigned int kk) const
    /* helper function to return a superdroplet's attributes
    at position kk in the initial conditions data. All 
    superdroplets created with same shared pointer to a 
    solute created in memory space of viewd_solute */
    {
      return {radii.at(kk), msols.at(kk), xis.at(kk), solutes(0)};
    }

  public:
    GenSuperdrop(const FetchInitData &fisd)
        : sdIdGen(std::make_unique<Superdrop::IDType::Gen>()),
          solutes("solute"),
          sdgbxindexes(fisd.sdgbxindex()),
          coord3s(fisd.coord3()),
          coord1s(fisd.coord1()),
          coord2s(fisd.coord2()),
          radii(fisd.radius()),
          msols(fisd.msol()),
          xis(fisd.xi())
    {
      /* all superdroplets reference same solute
      (in memory space of viewd_solute) */
      solutes(0) = std::make_shared<const SoluteProperties>();
    }

    Superdrop operator()(const unsigned int kk) const
    {
      const unsigned int sdgbxindex(sdgbxindexes.at(kk));
      const double coord3(coord3s.at(kk));
      const double coord1(coord1s.at(kk));
      const double coord2(coord2s.at(kk));
      const SuperdropAttrs attrs(attrs_at(kk));
      const auto sd_id = sdIdGen->next();

      return Superdrop(sdgbxindex, coord3,
                       coord1, coord2,
                       attrs, sd_id);
    }
  };

  viewd_supers initialise_supers() const
  /* initialise a view of superdrops (on device memory)
  using data from an InitData instance for their initial
  gbxindex, spatial coordinates and attributes */
  {
    const size_t totnsupers(fisd.get_totnsupers());
    const GenSuperdrop gen_superdrop(fisd);

    viewd_supers supers("supers", totnsupers);
    for (size_t kk(0); kk < totnsupers; ++kk)
    {
      supers(kk) = gen_superdrop(kk);
    }

    ensure_initialisation_complete(supers);

    return supers;
  }

  void ensure_initialisation_complete(viewd_constsupers supers) const
  /* ensure the number of superdrops in the view matches the
  size according to the initial conditions */
  {
    if (supers.extent(0) < fisd.get_size())
    {
      const std::string err("Fewer superdroplets were created than were"
                            " given by initialisation data ie. " +
                            std::to_string(supers.extent(0)) + " < " +
                            std::to_string(fisd.get_size()));
      throw std::invalid_argument(err);
    }
  }

  void print_supers(viewd_constsupers supers) const
  /* print superdroplet information */
  {
    for (size_t kk(0); kk < supers.extent(0); ++kk)
    {
      std::cout << "---"
                   "\nsdid: "
                << supers(kk).id.value
                << "\nsdgbxindex: " << supers(kk).get_sdgbxindex()
                << "\n---\n";
    }
  }

  viewd_supers sort_supers(viewd_supers supers) const
  /* sort the view of superdroplets by their sdgbxindexes */
  {
    return supers;
  }

public:
  CreateSupers(const FetchInitData &fisd) : fisd(fisd) {}

  viewd_supers operator()() const
  /* create view of "totnsupers" number of superdrops
  (in device memory) which is ordered by the superdrops'
  gridbox indexes using the initial conditions
  generated by the referenced FetchInitData type */
  {
    viewd_supers supers(initialise_supers());

    supers = sort_supers(supers);

    print_supers(supers);

    return supers;
  }
};

#endif // CREATESUPERS_HPP