#!/usr/bin/env python
# This test checks API properties of ManualPlacement::on_hicann
import unittest

import pyhmf as pynn

import pyhalbe, pymarocco
from pymarocco import PyMarocco
from pyhalco_common import Enum
from pyhalco_hicann_v2 import HICANNOnWafer

class PlacementAddTest(unittest.TestCase):

    def test(self):

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

if __name__ == "__main__":
    unittest.main()
