# This test checks API properties of ManualPlacement::on_hicann

import pyhmf as pynn

import pyhalbe, pymarocco
from pymarocco import PyMarocco
from pyhalbe.Coordinate import HICANNOnWafer, Enum


marocco = PyMarocco()

useOne = HICANNOnWafer(Enum(276))
useTwo = HICANNOnWafer(Enum(277))
use = [useOne, useTwo]
use_tpl = tuple(use)

# place a population to a single HICANN (scalar parameter)
pop = pynn.Population(10, pynn.IF_cond_exp, {})
marocco.manual_placement.on_hicann(pop, useOne)

# place a population onto multiple HICANNs
# using a Python list()
pop = pynn.Population(10, pynn.IF_cond_exp, {})
marocco.manual_placement.on_hicann(pop, use)

# place a population onto multiple HICANNs
# using a Python tuple()
pop = pynn.Population(10, pynn.IF_cond_exp, {})
marocco.manual_placement.on_hicann(pop, use_tpl)
