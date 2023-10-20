/*
 * ----- CLEO -----
 * File: printobserver.cpp
 * Project: observers
 * Created Date: Friday 13th October 2023
 * Author: Clara Bayley (CB)
 * Additional Contributors:
 * -----
 * Last Modified: Friday 20th October 2023
 * Modified By: CB
 * -----
 * License: BSD 3-Clause "New" or "Revised" License
 * https://opensource.org/licenses/BSD-3-Clause
 * -----
 * Copyright (c) 2023 MPI-M, Clara Bayley
 * -----
 * File Description:
 * Observer Concept and related structures for various ways
 * of observing (outputing data from) CLEO.
 * An example of an observer is printing some data
 * from a gridbox's thermostate to the terminal
 */


#include "./printobserver.hpp"

void PrintObserver::
    print_statement(const unsigned int t_mdl,
                    const viewh_constgbx h_gbxs) const
{
  constexpr int printprec(1); // precision to print data with
  
  const auto gbx = h_gbxs(0);
  std::cout << "t="
            << std::fixed << std::setprecision(printprec)
            << step2realtime(t_mdl)
            << "s, totnsupers=" << gbx.domaintotnsupers()
            << ", ngbxs=" << h_gbxs.extent(0)
            << ", (Gbx" << gbx.get_gbxindex()
            << ": [T, p, qv] = [" << gbx.state.temp * dlc::TEMP0
            << "K, " << gbx.state.press * dlc::P0
            << "Pa, " << gbx.state.qvap
            << "], nsupers = " << gbx.supersingbx.nsupers() << ")\n";
}