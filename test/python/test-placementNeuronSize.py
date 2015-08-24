import unittest
import pyhmf as pynn
import pyhalbe, pymarocco
from pymarocco import PyMarocco
from pyhalbe.Coordinate import HICANNGlobal, Enum

class PlacementNeuronSize(unittest.TestCase):
    """
    Tests regarding the neuron size in placement.
    """

    def test_defaultNeuronSize(self):
        """tests whether whether only the supported neuron sizes are allowed"""
        marocco = PyMarocco()
        p = marocco.placement

        for s in range(0,65):
            if (s%4==0 and s>0):
                p.setDefaultNeuronSize(s)
                self.assertEqual(s, p.getDefaultNeuronSize())
            else:
                self.assertRaises(RuntimeError, p.setDefaultNeuronSize, s)

        # not larger than 64
        self.assertRaises(RuntimeError, p.setDefaultNeuronSize, 65)
        self.assertRaises(RuntimeError, p.setDefaultNeuronSize, 68)

    def test_addWithNeuronSize(self):
        """tests manual placement with custom neuron size"""

        marocco = PyMarocco()

        useOne = HICANNGlobal(Enum(276))
        useTwo = HICANNGlobal(Enum(277))
        use = [useOne, useTwo]
        useList = pymarocco.Placement.List(use)

        # place a population to a single HICANN (scalar parameter)
        pop = pynn.Population(10, pynn.IF_cond_exp, {})
        marocco.placement.add(pop, useOne, 8)

        # place a population onto multiple HICANNs
        # using the pymarocco::Placement::List type
        pop = pynn.Population(10, pynn.IF_cond_exp, {})
        marocco.placement.add(pop, useList, 4)

        # only set the neuron size for the population, but no HICANN.
        marocco.placement.add(pop, 12)
        self.assertRaises(RuntimeError, marocco.placement.add, pop, 1)

if __name__ == '__main__':
    unittest.main()
