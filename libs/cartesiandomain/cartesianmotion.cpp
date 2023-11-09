/*
 * ----- CLEO -----
 * File: cartesianmotion.cpp
 * Project: cartesiandomain
 * Created Date: Wednesday 8th November 2023
 * Author: Clara Bayley (CB)
 * Additional Contributors:
 * -----
 * Last Modified: Thursday 9th November 2023
 * Modified By: CB
 * -----
 * License: BSD 3-Clause "New" or "Revised" License
 * https://opensource.org/licenses/BSD-3-Clause
 * -----
 * Copyright (c) 2023 MPI-M, Clara Bayley
 * -----
 * File Description:
 */

#include "./cartesianmotion.hpp"

KOKKOS_FUNCTION void
check_inbounds_or_outdomain(const unsigned int idx,
                            const Kokkos::pair<double, double> bounds,
                            const double coord);

KOKKOS_FUNCTION int
flag_sdgbxindex(const unsigned int idx,
                const Kokkos::pair<double, double> bounds,
                const double coord);

KOKKOS_FUNCTION unsigned int
update_if_coord3nghbr(const CartesianMaps &gbxmaps,
                          unsigned int idx,
                          Superdrop &drop);

KOKKOS_FUNCTION unsigned int
update_if_coord1nghbr(const CartesianMaps &gbxmaps,
                      unsigned int idx,
                      Superdrop &drop);

KOKKOS_FUNCTION unsigned int
update_if_coord2nghbr(const CartesianMaps &gbxmaps,
                      unsigned int idx,
                      Superdrop &drop);

KOKKOS_FUNCTION unsigned int
backwards_coord3idx(const unsigned int gbxindex,
                 const CartesianMaps &gbxmaps,
                 Superdrop &superdrop);

KOKKOS_FUNCTION unsigned int
forwards_coord3idx(const unsigned int gbxindex,
                const CartesianMaps &gbxmaps,
                Superdrop &drop);

KOKKOS_FUNCTION void
CartesianMotion::update_superdrop_gbxindex(const unsigned int gbxindex,
                                           const CartesianMaps &gbxmaps,
                                           Superdrop &drop) const
/* function satisfies requirements of
"update_superdrop_gbxindex" in the motion concept to update a
superdroplet if it should move between gridboxes in a
cartesian domain. For each direction (z, then x, then y),
superdrop coord is compared to gridbox bounds given by gbxmaps
for the current gbxindex 'idx'. If superdrop coord lies outside
bounds, forward or backward neighbour functions are called as
appropriate  to update sdgbxindex (and possibly other superdrop
attributes) */
{
  unsigned int idx(gbxindex);

  idx = update_if_coord3nghbr(gbxmaps, idx, drop);
  check_inbounds_or_outdomain(idx, gbxmaps.coord3bounds(idx),
                              drop.get_coord3());
  
  idx = update_if_coord1nghbr(gbxmaps, idx, drop);
  check_inbounds_or_outdomain(idx, gbxmaps.coord1bounds(idx),
                              drop.get_coord1());
  
  idx = update_if_coord2nghbr(gbxmaps, idx, drop);
  check_inbounds_or_outdomain(idx, gbxmaps.coord2bounds(idx),
                              drop.get_coord2());

  // current_gbxindex = update_ifneighbour(
  //                   gbxmaps,
  //                   backwards_neighbour = xbehind,
  //                   forwards_neighbour = xinfront,
  //                   get_bounds = gbxmaps.get_bounds_x(ii),
  //                   get_sdcoord = superdrop.coord1,
  //                   idx,
  //                   superdrop);

  // current_gbxindex = update_ifneighbour(
  //                   gbxmaps,
  //                   backwards_neighbour =  yleft,
  //                   forwards_neighbour =yright,
  //                   get_bounds = gbxmaps.get_bounds_y(ii),
  //                   get_sdcoord = superdrop.coord2,
  //                   idx,
  //                   superdrop);

  drop.set_sdgbxindex(idx);
}

KOKKOS_FUNCTION void
check_inbounds_or_outdomain(const unsigned int idx,
                            const Kokkos::pair<double, double> bounds,
                            const double coord)
/* raise error if superdrop not either out of domain 
or within bounds (ie. lower_bound <= coord < upper_bound) */
{
  const bool bad_gbxindex((idx != LIMITVALUES::uintmax) &&
                          ((coord < bounds.first) | (coord >= bounds.second)));

  assert((!bad_gbxindex) && "SD not in previous gbx nor a neighbour."
                            " Try reducing the motion timestep to"
                            " satisfy CFL criteria, or use "
                            " 'update_ifoutside' to update sd_gbxindex");
}

int flag_sdgbxindex(const unsigned int idx,
                    const Kokkos::pair<double, double> bounds,
                    const double coord)
/* returns flag to keep idx the same (flag = 0) or
update to forwards (flag = 1) or backwards (flag = 2)
neighbour. Flag = 0 if idx is out of domain value or 
if coord lies within bounds = {lowerbound, upperbound}.
(Note: lower bound inclusive and upper bound exclusive,
ie. lowerbound <= coord < upperbound).
Flag = 1 if coord < lowerbound, indicating idx should 
be updated to backwards neighbour.
Flag = 2 if coord >= upperbound, indicating idx should 
be updated to forwards neighbour. */
{
  if (idx == LIMITVALUES::uintmax)
  {
    return 0; // maintian idx that is already out of domain
  }
  else if (coord < bounds.first) // lowerbound
  {
    return 1; // idx -> backwards_neighbour
  }
  else if (coord >= bounds.second) // upperbound
  {
    return 2; // idx -> forwards_neighbour
  }
  else
  {
    return 0; // maintain idx if coord within bounds
  }
}

KOKKOS_FUNCTION unsigned int
update_if_coord3nghbr(const CartesianMaps &gbxmaps,
                        unsigned int idx,
                        Superdrop &drop)
/* return updated value of gbxindex in case superdrop should
move to neighbouring gridbox in coord3 direction. 
Funciton changes value of idx if flag != 0,
if flag = 1 idx updated to backwards neighbour gbxindex.
if flag = 2 idx updated to forwards neighbour gbxindex.
Note: backwards/forwards functions may change the 
superdroplet's attributes e.g. if it leaves the domain. */
{
  const int flag(flag_sdgbxindex(idx, gbxmaps.coord3bounds(idx),
                                 drop.get_coord3())); // if value != 0 idx needs to change
  switch (flag)
  {
  case 1:
    idx = backwards_coord3idx(idx, gbxmaps, drop);
    break;
  case 2:
     idx = forwards_coord3idx(idx, gbxmaps, drop);
    break;
  }
  return idx;
}

KOKKOS_FUNCTION unsigned int
update_if_coord1nghbr(const CartesianMaps &gbxmaps,
                          unsigned int idx,
                          Superdrop &drop)
/* return updated value of gbxindex in case superdrop should
move to neighbouring gridbox in coord1 direction. 
Funciton changes value of idx if flag != 0,
if flag = 1 idx updated to backwards neighbour gbxindex.
if flag = 2 idx updated to forwards neighbour gbxindex.
Note: backwards/forwards functions may change the 
superdroplet's attributes e.g. if it leaves the domain. */
{
  const int flag(flag_sdgbxindex(idx, gbxmaps.coord1bounds(idx),
                                 drop.get_coord1())); // if value != 0 idx needs to change
  switch (flag)
  {
  case 1:
    idx = backwards_coord1idx(idx, gbxmaps, drop);
    break;
  case 2:
     idx = forwards_coord1idx(idx, gbxmaps, drop);
    break;
  }
  return idx;
}

KOKKOS_FUNCTION unsigned int
update_if_coord2nghbr(const CartesianMaps &gbxmaps,
                          unsigned int idx,
                          Superdrop &drop)
/* return updated value of gbxindex in case superdrop should
move to neighbouring gridbox in coord2 direction. 
Funciton changes value of idx if flag != 0,
if flag = 1 idx updated to backwards neighbour gbxindex.
if flag = 2 idx updated to forwards neighbour gbxindex.
Note: backwards/forwards functions may change the 
superdroplet's attributes e.g. if it leaves the domain. */
{
  const int flag(flag_sdgbxindex(idx, gbxmaps.coord2bounds(idx),
                                 drop.get_coord2())); // if value != 0 idx needs to change
  switch (flag)
  {
  case 1:
    idx = backwards_coord2idx(idx, gbxmaps, drop);
    break;
  case 2:
     idx = forwards_coord2idx(idx, gbxmaps, drop);
    break;
  }
  return idx;
}

KOKKOS_FUNCTION unsigned int
backwards_coord3idx(const unsigned int idx,
                 const CartesianMaps &gbxmaps,
                 Superdrop &drop)
/* function to return gbxindex of neighbouring gridbox
in backwards coord3 (z) direction and to update superdrop
coord3 if superdrop has exceeded the z lower domain boundary */
{
  const unsigned int nghbr(gbxmaps.coord3backward(idx));

  if (at_cartesiandomainboundary(idx, 1, gbxmaps.get_ndim(0))) // SD was at lower z edge of domain (now moving beyond it)
  {
    const double lim1 = gbxmaps.coord3bounds(nghbr).second;   // upper lim of backward nghbour
    const double lim2 = gbxmaps.coord3bounds(idx).first; // lower lim of gbx
    drop.set_coord3(coord3_beyondz(drop.get_coord3(), lim1, lim2));
  }

  return nghbr; // gbxindex of zdown_neighbour
};

KOKKOS_FUNCTION unsigned int
forwards_coord3idx(const unsigned int idx,
                 const CartesianMaps &gbxmaps,
                 Superdrop &drop)
/* function to return gbxindex of neighbouring gridbox in
forwards coord3 (z) direction and to update superdrop coord3
if superdrop has exceeded the z upper domain boundary */
{
  const unsigned int nghbr(gbxmaps.coord3forward(idx));

  if (at_cartesiandomainboundary(idx + 1, 1, gbxmaps.get_ndim(0))) // SD was upper z edge of domain (now moving above it)
  {
    const double lim1 = gbxmaps.coord3bounds(nghbr).first; // lower lim of forward nghbour
    const double lim2 = gbxmaps.coord3bounds(idx).second; // upper lim of gbx
    drop.set_coord3(coord3_beyondz(drop.get_coord3(), lim1, lim2));
  }

  return nghbr; // gbxindex of zup_neighbour
};