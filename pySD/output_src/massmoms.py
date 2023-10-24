'''
----- CLEO -----
File: massmoms.py
Project: output_src
Created Date: Tuesday 24th October 2023
Author: Clara Bayley (CB)
Additional Contributors:
-----
Last Modified: Tuesday 24th October 2023
Modified By: CB
-----
License: BSD 3-Clause "New" or "Revised" License
https://opensource.org/licenses/BSD-3-Clause
-----
Copyright (c) 2023 MPI-M, Clara Bayley
-----
File Description:
python class to handle mass moments 
data from SDM zarr store in cartesian
domain
'''

import numpy as np
import xarray as xr

class MassMoments:

  def __init__(self, dataset, ntime, ndims, lab=""):
    
    ds = self.tryopen_dataset(dataset) 
    reshape = [ntime] + list(ndims)
    
    self.nsupers = self.var4d_fromzarr(ds, reshape, "n"+lab+"supers")      # number of superdroplets in gbxs over time
    self.mom0 = self.var4d_fromzarr(ds, reshape, "mom0"+lab)               # number of droplets in gbxs over time
    self.mom1 = self.var4d_fromzarr(ds, reshape, "mom1"+lab)               # total mass of droplets in gbxs over time
    self.mom2 = self.var4d_fromzarr(ds, reshape, "mom2"+lab)               # 2nd mass moment of droplets (~reflectivity)
    self.effmass = self.effective_mass()

    self.mom1_units = ds["mom1"].units                                # probably grams
    self.mom2_units = ds["mom2"].units                                # probably grams^2
    self.effmass_units = ds["mom2"].units + "/" + ds["mom1"].units    # probably grams

  def tryopen_dataset(self, dataset):
    
    if type(dataset) == str:
      print("mass moments dataset: ", dataset)
      return xr.open_dataset(dataset, engine="zarr", consolidated=False) 
    else:
      return dataset
  
  def var4d_fromzarr(self, ds, key):
    '''' returns 4D variable with dims
    [time, y, x, z] from zarr dataset "ds" '''
      
    return np.reshape(ds[key].values, self.reshape) 

  def effective_mass(self):
    ''' effective mass of droplets '''
    return self.mom2 / self.mom1      

  def __getitem__(self, key):
    if key == "nsupers":
      return self.nsupers
    elif key == "mom0":
      return self.mom0
    elif key == "mom1":
      return self.mom1
    elif key == "mom2":
      return self.mom2
    elif key == "effmass":
      return self.effmass
    else:
      err = "no known return provided for "+key+" key"
      raise ValueError(err)