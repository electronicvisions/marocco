import os
import unittest

from pyhalco_common import Enum
import pyhalco_hicann_v2 as C
import pyhmf as pynn
import pyredman

import utils


class TestRouting(utils.TestWithResults):
    def test_dijkstra_routing(self):
        """
        Integration test for Dijkstra-based L1 routing.

        Sets up a convoluted case that requires a convex route (which would
        not work using the backbone router).

         .------->------+
        167 168 169 170 |
                    206 v
            240 241 242 |
             ^---<------+
        """
        pynn.setup(marocco=self.marocco)

        source = pynn.Population(1, pynn.IF_cond_exp, {})
        target = pynn.Population(1, pynn.IF_cond_exp, {})
        proj = pynn.Projection(
            source, target, pynn.AllToAllConnector(weights=0.004))

        source_hicann = C.HICANNOnWafer(Enum(167))
        target_hicann = C.HICANNOnWafer(Enum(240))
        self.marocco.manual_placement.on_hicann(source, source_hicann)
        self.marocco.manual_placement.on_hicann(target, target_hicann)

        allowed_hicanns = [206] + list(range(167, 171)) + list(range(240, 243))
        wafer = self.marocco.default_wafer
        self.marocco.defects.set(pyredman.Wafer())
        for hicann in C.iter_all(C.HICANNOnWafer):
            if hicann.toEnum().value() in allowed_hicanns:
                continue
            self.marocco.defects.wafer().hicanns().disable(C.HICANNGlobal(hicann, wafer))

        self.marocco.l1_routing.algorithm(self.marocco.l1_routing.dijkstra)

        pynn.run(0)
        pynn.end()

        results = self.load_results()

        synapses = results.synapse_routing.synapses()
        self.assertEqual(1, synapses.size())


if __name__ == '__main__':
    unittest.main()
