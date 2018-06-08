import unittest, random, exceptions
from pymarocco import *
from pyhalbe.Coordinate import *
from pyhmf import *

class TestPyMarocco(unittest.TestCase):

    # test interface and insertions of manual placements
    def test_ManualPlacement(self):
        marocco = PyMarocco()
        placement = marocco.placement


        N = 10

        pops = []
        for ii in range(N):
            p = Population(random.randint(1, 10), EIF_cond_exp_isfa_ista)
            placement.add(p, HICANNGlobal())
            pops.append(p)

        self.assertEqual(N, len(placement.iter()))

        connector = AllToAllConnector(
                allow_self_connections=True,
                weights=1.)

        proj = Projection(pops[0], pops[1], connector, target='excitatory')

        with self.assertRaises(exceptions.IndexError) as e:
            marocco.stats.getWeights(proj)


if __name__ == '__main__':
    unittest.main()
