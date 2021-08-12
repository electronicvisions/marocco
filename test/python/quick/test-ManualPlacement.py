import unittest

from pymarocco.coordinates import LogicalNeuron
from pyhalco_common import X, Y, Enum
import pyhalco_hicann_v2 as C
import pyhmf as pynn

import utils


class TestManualPlacement(utils.TestWithResults):

    @utils.parametrize([1,2,3,4,5])
    def test_popview_combinations(self, view_number):
        # tests all possible combinations of mask lengths for different number of PopulationViews
        import pylogging
        from pymarocco import PyMarocco, Defects
        from pymarocco.results import Marocco
        pop_size = 5
        hicanns = [C.HICANNOnWafer(Enum(180 + view)) for view in range(view_number)]
        # generate possible mask lengths for Population Views
        pool = tuple(i for i in range(1, pop_size - view_number + 2))
        # generate all possible mask lengths for each PopulationView for a given total number of neurons
        # [[lengths_of_Popviews],number_of_used_neurons]
        view_lengths = [([], 0)]
        for _ in range(view_number):
            view_lengths = [(x+[y], csum+y) for x, csum in view_lengths for y in pool if csum <= pop_size - y]
        neurons = list(range(pop_size))
        for length in view_lengths:
            marocco = PyMarocco()
            marocco.backend = PyMarocco.Without
            marocco.persist = "results.bin"
            marocco.defects.backend = Defects.Backend.Without
            neuron_size = 4
            marocco.neuron_placement.default_neuron_size(neuron_size)
            pynn.setup(marocco=marocco)

            pop = pynn.Population(pop_size, pynn.IF_cond_exp, {})
            pop_views = []
            index = 0
            for view in range(view_number):
                # generate PopulationViews with all possible mask lengths
                # no permutations of neurons are tested
                pop_views.append(pynn.PopulationView(pop,neurons[index:index+length[0][view]]))
                marocco.manual_placement.on_hicann(pop_views[view],hicanns[view])
                index += length[0][view]

            pynn.run(0)
            pynn.end()
            results = Marocco.from_file(marocco.persist)

            for view in range(view_number):
                for nrn in pop_views[view]:
                    placement_item, = results.placement.find(nrn)
                    logical_neuron = placement_item.logical_neuron()
                    for denmem in logical_neuron:
                        self.assertEqual(hicanns[view], denmem.toHICANNOnWafer())

    def test_popview_external_source(self):
        pynn.setup(marocco=self.marocco)
        neuron_size = 4
        self.marocco.neuron_placement.default_neuron_size(neuron_size)

        size = 10
        pop_ext = pynn.Population(size, pynn.SpikeSourcePoisson, {'rate':2})
        pop_ext_1 = pynn.Population(size, pynn.SpikeSourcePoisson, {'rate':2})
        pop = pynn.Population(size, pynn.IF_cond_exp, {})
        connector = pynn.AllToAllConnector(weights=1)
        projections = [
            pynn.Projection(pop_ext, pop, connector, target='excitatory'),
            pynn.Projection(pop_ext_1, pop, connector, target='excitatory'),
        ]
        hicann = C.HICANNOnWafer(Enum(121))
        hicann_1 = C.HICANNOnWafer(Enum(122))

        pop_view = pynn.PopulationView(pop_ext,list(range(1,size,2)))
        pop_view_1 = pynn.PopulationView(pop_ext,list(range(0,size,2)))
        pop_1_view = pynn.PopulationView(pop_ext_1,list(range(1,size/2)))
        pop_1_view_1 = pynn.PopulationView(pop_ext_1,list(range(size-2,size/2,-1)))
        pop_1_auto_placement = pynn.PopulationView(pop_ext_1,[0,size/2,size-1])

        self.marocco.manual_placement.on_hicann(pop_view, hicann)
        self.marocco.manual_placement.on_hicann(pop_view_1, hicann_1)
        self.marocco.manual_placement.on_hicann(pop_1_view, hicann)
        self.marocco.manual_placement.on_hicann(pop_1_view_1, hicann_1)

        pynn.run(0)
        pynn.end()

        results = self.load_results()

        for nrn in pop_view:
            placement_item, = results.placement.find(nrn)
            self.assertEqual(hicann, placement_item.dnc_merger().toHICANNOnWafer())
        for nrn in pop_view_1:
            placement_item, = results.placement.find(nrn)
            self.assertEqual(hicann_1, placement_item.dnc_merger().toHICANNOnWafer())
        for nrn in pop_1_view:
            placement_item, = results.placement.find(nrn)
            self.assertEqual(hicann, placement_item.dnc_merger().toHICANNOnWafer())
        for nrn in pop_1_view_1:
            placement_item, = results.placement.find(nrn)
            self.assertEqual(hicann_1, placement_item.dnc_merger().toHICANNOnWafer())
        for nrn in pop_1_auto_placement:
            placement_item, = results.placement.find(nrn)
            self.assertIsNotNone(placement_item.dnc_merger().toHICANNOnWafer())

    def test_popview_on_neuron(self):
        pynn.setup(marocco=self.marocco)

        pop = pynn.Population(4, pynn.IF_cond_exp, {})
        neuron_block = C.NeuronBlockOnWafer(C.NeuronBlockOnHICANN(3))
        neuron_block_1 = C.NeuronBlockOnWafer(C.NeuronBlockOnHICANN(2))
        logical_neuron = (LogicalNeuron.on(neuron_block)
                          .add(C.NeuronOnNeuronBlock(X(3), Y(0)), 2)
                          .add(C.NeuronOnNeuronBlock(X(3), Y(1)), 2)
                          .done())
        logical_neuron_1 = (LogicalNeuron.on(neuron_block_1)
                          .add(C.NeuronOnNeuronBlock(X(4), Y(0)), 2)
                          .add(C.NeuronOnNeuronBlock(X(4), Y(1)), 2)
                          .done())

        popview = pynn.PopulationView(pop,[0])
        popview_1 = pynn.PopulationView(pop,[2])
        popview_auto_placement= pynn.PopulationView(pop,[1,3])
        self.marocco.manual_placement.on_neuron(popview, logical_neuron)
        self.marocco.manual_placement.on_neuron(popview_1, logical_neuron_1)

        pynn.run(0)
        pynn.end()

        results = self.load_results()

        placement_item, = results.placement.find(popview[0])
        self.assertEqual(logical_neuron, placement_item.logical_neuron())
        placement_item, = results.placement.find(popview_1[0])
        self.assertEqual(logical_neuron_1, placement_item.logical_neuron())
        for nrn in popview_auto_placement:
            placement_item, = results.placement.find(nrn)
            self.assertIsNotNone(placement_item.logical_neuron())

    @utils.parametrize([10, 100, 999])
    def test_popview_on_hicann(self, size):
        pynn.setup(marocco=self.marocco)
        neuron_size = 4
        self.marocco.neuron_placement.default_neuron_size(neuron_size)
        hicann = C.HICANNOnWafer(Enum(122))
        hicann_1 = C.HICANNOnWafer(Enum(123))
        hicann_2 = C.HICANNOnWafer(Enum(124))
        hicann_3 = C.HICANNOnWafer(Enum(125))
        pop = pynn.Population(size, pynn.IF_cond_exp, {})
        pop_1 = pynn.Population(size, pynn.IF_cond_exp, {})
        pop_view = pynn.PopulationView(pop,list(range(0,size,2)))
        pop_view_1 = pynn.PopulationView(pop,list(range(1,size,2)))
        pop_1_view = pynn.PopulationView(pop_1,list(range(1,size/2)))
        pop_1_view_1 = pynn.PopulationView(pop_1,list(range(size-2,size/2,-1)))
        pop_auto_placement = pynn.PopulationView(pop_1,[0,size/2,size-1])
        self.marocco.manual_placement.on_hicann(pop_view, hicann)
        self.marocco.manual_placement.on_hicann(pop_view_1, hicann_1)
        self.marocco.manual_placement.on_hicann(pop_1_view, hicann_2)
        self.marocco.manual_placement.on_hicann(pop_1_view_1, hicann_3)

        if neuron_size * size/2 > C.NeuronOnHICANN.enum_type.size:
            with self.assertRaises(RuntimeError):
                pynn.run(0)
                pynn.end()
            return

        pynn.run(0)
        pynn.end()

        results = self.load_results()

        for nrn in pop_view:
            placement_item, = results.placement.find(nrn)
            logical_neuron = placement_item.logical_neuron()
            for denmem in logical_neuron:
                self.assertEqual(hicann, denmem.toHICANNOnWafer())

        for nrn in pop_view_1:
            placement_item, = results.placement.find(nrn)
            logical_neuron = placement_item.logical_neuron()
            for denmem in logical_neuron:
                self.assertEqual(hicann_1, denmem.toHICANNOnWafer())

        for nrn in pop_1_view:
            placement_item, = results.placement.find(nrn)
            logical_neuron = placement_item.logical_neuron()
            for denmem in logical_neuron:
                self.assertEqual(hicann_2, denmem.toHICANNOnWafer())

        for nrn in pop_1_view_1:
            placement_item, = results.placement.find(nrn)
            logical_neuron = placement_item.logical_neuron()
            for denmem in logical_neuron:
                self.assertEqual(hicann_3, denmem.toHICANNOnWafer())

        for nrn in pop_auto_placement:
            placement_item, = results.placement.find(nrn)
            logical_neuron = placement_item.logical_neuron()
            for denmem in logical_neuron:
                self.assertIsNotNone(denmem.toHICANNOnWafer())

    @utils.parametrize([1, 2, 10])
    def test_same_popview_on_hicann(self, size):
        pynn.setup(marocco=self.marocco)
        neuron_size = 4
        self.marocco.neuron_placement.default_neuron_size(neuron_size)
        hicann = C.HICANNOnWafer(Enum(123))
        hicann_1 = C.HICANNOnWafer(Enum(122))
        pop = pynn.Population(size, pynn.IF_cond_exp, {})
        pop_view = pynn.PopulationView(pop,[0])
        pop_view_1 = pynn.PopulationView(pop,[0])
        self.marocco.manual_placement.on_hicann(pop_view, hicann)
        self.marocco.manual_placement.on_hicann(pop_view_1, hicann_1)

        with self.assertRaises(RuntimeError):
            pynn.run(0)
            pynn.end()

    @utils.parametrize([1, 2, 10, 100, 999])
    def test_on_hicann(self, size):
        pynn.setup(marocco=self.marocco)
        neuron_size = 4
        self.marocco.neuron_placement.default_neuron_size(neuron_size)

        hicann = C.HICANNOnWafer(Enum(123))
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

    @utils.parametrize([1, 2, 10, 100, 999])
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

        for pop, size in sizes.items():
            placement_item, = results.placement.find(pop[0])
            logical_neuron = placement_item.logical_neuron()
            self.assertEqual(size, logical_neuron.size())

    def test_on_neuron(self):
        pynn.setup(marocco=self.marocco)

        pop = pynn.Population(1, pynn.IF_cond_exp, {})
        neuron_block = C.NeuronBlockOnWafer(C.NeuronBlockOnHICANN(3))
        logical_neuron = (LogicalNeuron.on(neuron_block)
                          .add(C.NeuronOnNeuronBlock(X(3), Y(0)), 2)
                          .add(C.NeuronOnNeuronBlock(X(3), Y(1)), 2)
                          .done())
        self.marocco.manual_placement.on_neuron(pop, logical_neuron)

        pynn.run(0)
        pynn.end()

        results = self.load_results()

        placement_item, = results.placement.find(pop[0])
        self.assertEqual(logical_neuron, placement_item.logical_neuron())

    def test_on_rectangular_neuron(self):
        pynn.setup(marocco=self.marocco)

        pop = pynn.Population(1, pynn.IF_cond_exp, {})
        topleft = C.NeuronOnWafer(C.NeuronOnHICANN(X(2), Y(0)))
        logical_neuron = LogicalNeuron.rectangular(topleft, size=4)
        self.marocco.manual_placement.on_neuron(pop, logical_neuron)

        pynn.run(0)
        pynn.end()

        results = self.load_results()

        placement_item, = results.placement.find(pop[0])
        self.assertEqual(logical_neuron, placement_item.logical_neuron())

    @utils.parametrize([1, 2, 3])
    def test_on_neurons(self, pop_size):
        pynn.setup(marocco=self.marocco)

        pop = pynn.Population(pop_size, pynn.IF_cond_exp, {})
        neuron_block = C.NeuronBlockOnWafer(C.NeuronBlockOnHICANN(3))
        logical_neurons = [
            (LogicalNeuron.on(neuron_block)
             .add(C.NeuronOnNeuronBlock(X(3), Y(0)), 2)
             .add(C.NeuronOnNeuronBlock(X(3), Y(1)), 2)
             .done()),
            (LogicalNeuron.on(neuron_block)
             .add(C.NeuronOnNeuronBlock(X(11), Y(0)), 2)
             .add(C.NeuronOnNeuronBlock(X(11), Y(1)), 2)
             .done()),
        ]
        self.marocco.manual_placement.on_neuron(pop, logical_neurons)

        if pop_size != len(logical_neurons):
            with self.assertRaises(RuntimeError):
                pynn.run(0)
                pynn.end()
            return

        pynn.run(0)
        pynn.end()

        results = self.load_results()

        for nrn, logical_neuron in zip(pop, logical_neurons):
            placement_item, = results.placement.find(nrn)
            self.assertEqual(logical_neuron, placement_item.logical_neuron())

    def test_on_neuron_wrong_shape(self):
        pynn.setup(marocco=self.marocco)

        pop = pynn.Population(1, pynn.IF_cond_exp, {})
        neuron_block = C.NeuronBlockOnWafer(C.NeuronBlockOnHICANN(3))
        logical_neuron = (LogicalNeuron.on(neuron_block)
                          .add(C.NeuronOnNeuronBlock(X(2), Y(0)), 4)
                          .add(C.NeuronOnNeuronBlock(X(3), Y(1)), 2)
                          .done())
        self.marocco.manual_placement.on_neuron(pop, logical_neuron)

        with self.assertRaises(RuntimeError):
            pynn.run(0)
            pynn.end()


if __name__ == '__main__':
    unittest.main()
