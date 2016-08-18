import unittest

from pymarocco.coordinates import LogicalNeuron
import pyhalbe.Coordinate as C
import pyhmf as pynn

import utils


class TestManualPlacement(utils.TestWithResults):
    @utils.parametrize([1, 2, 10, 999])
    def test_on_hicann(self, size):
        pynn.setup(marocco=self.marocco)
        neuron_size = 4
        self.marocco.neuron_placement.default_neuron_size(neuron_size)

        hicann = C.HICANNOnWafer(C.Enum(123))
        pop = pynn.Population(size, pynn.IF_cond_exp, {})
        self.marocco.manual_placement.on_hicann(pop, hicann)

        if neuron_size * size > C.NeuronOnHICANN.enum_type.size:
            with self.assertRaises(RuntimeError):
                pynn.run(0)
                pynn.end()
            return

        pynn.run(0)
        pynn.end()

        results = self.load_results()

        for nrn in pop:
            placement_item, = results.placement.find(nrn)
            logical_neuron = placement_item.logical_neuron()
            self.assertEqual(neuron_size, logical_neuron.size())
            for denmem in logical_neuron:
                self.assertEqual(hicann, denmem.toHICANNOnWafer())

    @utils.parametrize([1, 2, 10, 999])
    def test_on_neuron_block(self, size):
        pynn.setup(marocco=self.marocco)
        neuron_size = 4
        self.marocco.neuron_placement.default_neuron_size(neuron_size)

        neuron_block = C.NeuronBlockOnWafer(C.NeuronBlockOnHICANN(3))
        pop = pynn.Population(size, pynn.IF_cond_exp, {})
        self.marocco.manual_placement.on_neuron_block(pop, neuron_block)

        if neuron_size * size > C.NeuronOnNeuronBlock.enum_type.size:
            with self.assertRaises(RuntimeError):
                pynn.run(0)
                pynn.end()
            return

        pynn.run(0)
        pynn.end()

        results = self.load_results()

        for nrn in pop:
            placement_item, = results.placement.find(nrn)
            logical_neuron = placement_item.logical_neuron()
            self.assertEqual(neuron_size, logical_neuron.size())
            for denmem in logical_neuron:
                self.assertEqual(neuron_block, denmem.toNeuronBlockOnWafer())

    def test_with_size(self):
        pynn.setup(marocco=self.marocco)
        default_size = 4
        self.marocco.neuron_placement.default_neuron_size(default_size)

        sizes = {}

        pop = pynn.Population(1, pynn.IF_cond_exp, {})
        sizes[pop] = default_size

        pop = pynn.Population(1, pynn.IF_cond_exp, {})
        self.marocco.manual_placement.with_size(pop, 2)
        sizes[pop] = 2

        pop = pynn.Population(1, pynn.IF_cond_exp, {})
        self.marocco.manual_placement.with_size(pop, 6)
        sizes[pop] = 6

        pynn.run(0)
        pynn.end()

        results = self.load_results()

        for pop, size in sizes.iteritems():
            placement_item, = results.placement.find(pop[0])
            logical_neuron = placement_item.logical_neuron()
            self.assertEqual(size, logical_neuron.size())


if __name__ == '__main__':
    unittest.main()
