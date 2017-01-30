import unittest
import pyhmf as pynn
import pyhalbe, pymarocco
from pymarocco import PyMarocco
from pyhalbe.Coordinate import HICANNOnWafer, Enum

class PlacementNeuronSize(unittest.TestCase):
    """
    Tests regarding the neuron size in placement.
    """

    def test_defaultNeuronSize(self):
        """tests whether whether only the supported neuron sizes are allowed"""
        marocco = PyMarocco()
        p = marocco.neuron_placement

        # has to be larger than zero
        self.assertRaises(ValueError, p.default_neuron_size, -1)
        self.assertRaises(ValueError, p.default_neuron_size, 0)

        for s in range(1, 65):
            if (s % 2 == 0):
                p.default_neuron_size(s)
                self.assertEqual(s, p.default_neuron_size())
            else:
                self.assertRaises(ValueError, p.default_neuron_size, s)

        # not larger than 64
        self.assertRaises(ValueError, p.default_neuron_size, 65)
        self.assertRaises(ValueError, p.default_neuron_size, 68)

    def test_addWithNeuronSize(self):
        """tests manual placement with custom neuron size"""

        marocco = PyMarocco()

        useOne = HICANNOnWafer(Enum(276))
        useTwo = HICANNOnWafer(Enum(277))
        use = [useOne, useTwo]

        # place a population to a single HICANN (scalar parameter)
        pop = pynn.Population(10, pynn.IF_cond_exp, {})
        marocco.manual_placement.on_hicann(pop, useOne, 8)

        # place a population onto multiple HICANNs
        pop = pynn.Population(10, pynn.IF_cond_exp, {})
        marocco.manual_placement.on_hicann(pop, use, 4)

        # only set the neuron size for the population, but no HICANN.
        marocco.manual_placement.with_size(pop, 12)

        self.assertRaises(
            ValueError, marocco.manual_placement.with_size, pop, 1)

if __name__ == '__main__':
    unittest.main()
