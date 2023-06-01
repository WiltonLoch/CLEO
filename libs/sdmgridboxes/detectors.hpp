// Author: Clara Bayley
// File: "detectors.hpp"
/* Header file for functions and
structures related to detectors
(e.g. of SDM processes) in gridboxes
which copy data from detections
into 'logbooks' */

#ifndef DETECTORS_HPP
#define DETECTORS_HPP

#include <limits>
#include <memory>
#include <vector>

#include "./logbooks.hpp"
#include "superdrop_solver/superdrop.hpp"

using dblLogbook = std::shared_ptr<Logbook<double>>;

struct AccumPrecipDetector
/* detector which stores the value of
accumulated precipitation in an entry of
a logbook controlled by the
EntryInLogbook instance */
{
private:
  EntryInLogbook<double> manage_entry;

  double accumulated_precipitation(const Superdrop drop)
  {
    return 0.0;
  }

public:
  void create_entry_in_logbook(const dblLogbook logbook,
                               const unsigned int gbxindex)
  /* use the manage_entry to create an entry in logbook */
  {
    manage_entry.create_entry(logbook, gbxindex);
  }

  void operator()(const Superdrop drop)
  {
    if (manage_entry.get_logbook())
    {
      manage_entry(accumulated_precipitation(drop));
    }
  }
};

class Detectors
/* Detectors stores various detector types and 
a reference to logbook instances found in
'DetectionLogbooks'. Detectors is interface to 
control use of detectors (and logbooks) by
a gridbox */
{
private:
  const DetectionLogbooks &logbooks;
  
  AccumPrecipDetector accpp_dtr;

public:
  Detectors(const DetectionLogbooks &logbooks)
      : logbooks(logbooks) {}

  void install_accumprecip_detector(const unsigned int gbxindex)
  /* install accumulated precipitation detector by creating
  and entry in the accpp logbook with tag 'gbxindex' */
  {
    accpp_dtr.create_entry_in_logbook(logbooks.accpp, gbxindex);
  }
};

std::unique_ptr<Detectors> install_detectors(const unsigned int gbxindex,
                                             const DetectionLogbooks &logbooks,
                                             const Maps4GridBoxes &gbxmaps)
{
  auto detectors = std::make_unique<Detectors>(logbooks);

  if (gbxmaps.get_bounds_z(gbxindex).second < zLIM_HERE)
  {
    detectors->install_accumprecip_detector(gbxindex);
  }

  return detectors;
}

#endif // DETECTORS_HPP