import unittest

from pymarocco.coordinates import LogicalNeuron
import pyhalbe.Coordinate as C
import pyhmf as pynn

import utils


class TestMergerRouting(utils.TestWithResults):
    def test_min_spl1_is_nongreedy(self):
        """
        When placing a single population to a HICANN there should still be room for external input.
        Previously the default merger routing strategy (min SPL1) merged as many adjacent neuron
        blocks as possible.  In doing this, the NeuronBlockOnHICANN(7) was connected to
        DNCMergerOnHICANN(3).
        This prevented the external input to be placed to DNCMergerOnHICANN(7) of the same HICANN,
        since the corresponding background generator could not be connected.
        """
        pynn.setup(marocco=self.marocco)
        neuron_size = 4
        self.marocco.neuron_placement.default_neuron_size(neuron_size)
        self.marocco.merger_routing.strategy(
            self.marocco.merger_routing.minimize_number_of_sending_repeaters)
        self.marocco.neuron_placement.restrict_rightmost_neuron_blocks(True)

        hicann = C.HICANNOnWafer(C.Enum(123))
        pop = pynn.Population(1, pynn.IF_cond_exp, {})
        self.marocco.manual_placement.on_hicann(pop, hicann)
        in_pop = pynn.Population(1, pynn.SpikeSourceArray, {})
        self.marocco.manual_placement.on_hicann(in_pop, hicann)

        pynn.run(0)
        pynn.end()

        results = self.load_results()

        nrn = pop[0]
        placement_item, = results.placement.find(nrn)
        logical_neuron = placement_item.logical_neuron()
        self.assertEqual(neuron_size, logical_neuron.size())
        for denmem in logical_neuron:
            self.assertEqual(hicann, denmem.toHICANNOnWafer())

        nrn = in_pop[0]
        placement_item, = results.placement.find(nrn)
        logical_neuron = placement_item.logical_neuron()
        self.assertTrue(logical_neuron.is_external())
        self.assertEqual(hicann, placement_item.address().toHICANNOnWafer())

    @utils.parametrize([(0,), (1,), (0, 1), (1, 2), (4,), (4, 6), (3,), (5,), range(7)])
    def test_min_spl1_is_nongreedy_when_pops_are_placed_to_nbs(self, nbs):
        """
        See above.  Instead of a single population placed to the HICANN, populations are placed to
        specific neuron blocks.
        """
        pynn.setup(marocco=self.marocco)
        neuron_size = 4
        self.marocco.neuron_placement.default_neuron_size(neuron_size)
        self.marocco.merger_routing.strategy(
            self.marocco.merger_routing.minimize_number_of_sending_repeaters)
        self.marocco.neuron_placement.restrict_rightmost_neuron_blocks(True)

        hicann = C.HICANNOnWafer(C.Enum(123))
        pops = []
        for nb in nbs:
            pop = pynn.Population(1, pynn.IF_cond_exp, {})
            self.marocco.manual_placement.on_neuron_block(
                pop, C.NeuronBlockOnWafer(C.NeuronBlockOnHICANN(nb), hicann))
            pops.append(pop)
        in_pop = pynn.Population(1, pynn.SpikeSourceArray, {})
        self.marocco.manual_placement.on_hicann(in_pop, hicann)

        pynn.run(0)
        pynn.end()

        results = self.load_results()

        for pop in pops:
            nrn = pop[0]
            placement_item, = results.placement.find(nrn)
            logical_neuron = placement_item.logical_neuron()
            self.assertEqual(neuron_size, logical_neuron.size())
            for denmem in logical_neuron:
                self.assertEqual(hicann, denmem.toHICANNOnWafer())
            address = placement_item.address()

            # All used neuron blocks should still be connected to a single DNC merger.
            dnc = C.DNCMergerOnHICANN(3)
            self.assertEqual(hicann, address.toHICANNOnWafer())
            self.assertEqual(dnc, address.toDNCMergerOnHICANN())
            self.assertEqual(
                C.DNCMergerOnWafer(dnc, hicann), address.toDNCMergerOnWafer())

        nrn = in_pop[0]
        placement_item, = results.placement.find(nrn)
        logical_neuron = placement_item.logical_neuron()
        self.assertTrue(logical_neuron.is_external())
        address = placement_item.address()

        # External input should be on the rightmost DNC merger since that is tried first.
        dnc = C.DNCMergerOnHICANN(7)
        self.assertEqual(hicann, address.toHICANNOnWafer())
        self.assertEqual(dnc, address.toDNCMergerOnHICANN())
        self.assertEqual(
            C.DNCMergerOnWafer(dnc, hicann), address.toDNCMergerOnWafer())

    def test_min_spl1_should_allow_external_input_on_same_chip(self):
        """
        Even when the rightmost neuron block / DNC merger is not reserved for external input, it
        should be possible to place external input on the same chip.
        """
        pynn.setup(marocco=self.marocco)
        neuron_size = 4
        self.marocco.neuron_placement.default_neuron_size(neuron_size)
        self.marocco.merger_routing.strategy(
            self.marocco.merger_routing.minimize_number_of_sending_repeaters)
        # Do not reserve rightmost neuron block / DNC merger for external input.
        self.marocco.neuron_placement.restrict_rightmost_neuron_blocks(False)

        hicann = C.HICANNOnWafer(C.Enum(123))
        pops = []
        # All but the first neuron block are occupied.
        for nb in range(1, C.NeuronBlockOnHICANN.end):
            pop = pynn.Population(1, pynn.IF_cond_exp, {})
            self.marocco.manual_placement.on_neuron_block(
                pop, C.NeuronBlockOnWafer(C.NeuronBlockOnHICANN(nb), hicann))
            pops.append(pop)
        in_pop = pynn.Population(1, pynn.SpikeSourceArray, {})
        self.marocco.manual_placement.on_hicann(in_pop, hicann)

        pynn.run(0)
        pynn.end()

        results = self.load_results()

        for pop in pops:
            nrn = pop[0]
            placement_item, = results.placement.find(nrn)
            logical_neuron = placement_item.logical_neuron()
            self.assertEqual(neuron_size, logical_neuron.size())
            for denmem in logical_neuron:
                self.assertEqual(hicann, denmem.toHICANNOnWafer())
            address = placement_item.address()

            # All used neuron blocks should be connected to a single DNC merger.
            dnc = C.DNCMergerOnHICANN(3)
            self.assertEqual(hicann, address.toHICANNOnWafer())
            self.assertEqual(dnc, address.toDNCMergerOnHICANN())
            self.assertEqual(
                C.DNCMergerOnWafer(dnc, hicann), address.toDNCMergerOnWafer())

        nrn = in_pop[0]
        placement_item, = results.placement.find(nrn)
        logical_neuron = placement_item.logical_neuron()
        self.assertTrue(logical_neuron.is_external())
        address = placement_item.address()

        # External input should be on the leftmost DNC merger, since all other
        # mergers do not have direct access to a background generator.
        dnc = C.DNCMergerOnHICANN(0)
        self.assertEqual(hicann, address.toHICANNOnWafer())
        self.assertEqual(dnc, address.toDNCMergerOnHICANN())
        self.assertEqual(
            C.DNCMergerOnWafer(dnc, hicann), address.toDNCMergerOnWafer())


if __name__ == '__main__':
    unittest.main()
