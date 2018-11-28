import unittest, random, exceptions
from pymarocco import *
from pyhalbe.Coordinate import *
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

        with self.assertRaises(exceptions.IndexError) as e:
            marocco.stats.getWeights(proj)

    def test_hicann_configurator(self):
        marocco = PyMarocco()
        with self.assertRaises(TypeError):
            marocco.hicann_configurator
        import pysthal
        marocco.hicann_configurator = pysthal.HICANNConfigurator()
        self.assertTrue(isinstance(marocco.hicann_configurator, pysthal.HICANNConfigurator))


if __name__ == '__main__':
    unittest.main()
