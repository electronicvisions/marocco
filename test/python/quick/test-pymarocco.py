import unittest, random, builtins
from pymarocco import *
from pyhalco_hicann_v2 import *
import pyhmf as pynn

class TestPyMarocco(unittest.TestCase):

    # test interface and insertions of manual placements
    def test_ManualPlacement(self):
        marocco = PyMarocco()

        N = 10

        pops = []
        for ii in range(N):
            p = pynn.Population(random.randint(1, 10), pynn.EIF_cond_exp_isfa_ista)
            marocco.manual_placement.on_hicann(p, HICANNOnWafer())
            pops.append(p)

        connector = pynn.AllToAllConnector(
                allow_self_connections=True,
                weights=1.)

        proj = pynn.Projection(pops[0], pops[1], connector, target='excitatory')

        with self.assertRaises(builtins.IndexError) as e:
            marocco.stats.getWeights(proj)


if __name__ == '__main__':
    unittest.main()
