# This test checks API properties of pymarocco::Placement::add

import pyhmf as pynn

import pyhalbe, pymarocco
from pymarocco import PyMarocco
from pyhalbe.Coordinate import HICANNGlobal, Enum


marocco = PyMarocco()

useOne = HICANNGlobal(Enum(276))
useTwo = HICANNGlobal(Enum(277))
use = [useOne, useTwo]
use_tpl = tuple(use)
useList = pymarocco.Placement.List(use)

# place a population to a single HICANN (scalar parameter)
pop = pynn.Population(10, pynn.IF_cond_exp, {})
marocco.placement.add(pop, useOne)

# place a population onto multiple HICANNs
# using the pymarocco::Placement::List type
pop = pynn.Population(10, pynn.IF_cond_exp, {})
marocco.placement.add(pop, useList)

# place a population onto multiple HICANNs
# using a Python list()
pop = pynn.Population(10, pynn.IF_cond_exp, {})
marocco.placement.add(pop, use)

# place a population onto multiple HICANNs
# using a Python tuple()
pop = pynn.Population(10, pynn.IF_cond_exp, {})
marocco.placement.add(pop, use_tpl)
