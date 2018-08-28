import unittest
import pyhmf as pynn
from pymarocco_runtime import ClusterByPopulationConnectivity as placer
import pyhalbe.Coordinate as C

import pylogging
import utils


class PlacementAlgorithmsClusterPop(utils.TestWithResults):
    """
    Tests python switches of placement algorithm.
    """
    pops = []

    def run_placement(self):
        """
        create 10 populations with 1 neuron, connected in a chain
        """
        pylogging.set_loglevel(pylogging.get("marocco"),
                               pylogging.LogLevel.TRACE)
        pylogging.set_loglevel(pylogging.get("Calibtic"),
                               pylogging.LogLevel.ERROR)

        self.pops = []
        for i in range(10):
            pop = pynn.Population(1, pynn.IF_cond_exp, {})
            self.pops.append(pop)
            if(i > 1):
                proj = pynn.Projection(self.pops[i-1],
                                       self.pops[i],
                                       pynn.AllToAllConnector(weights=0.01))
                proj  # prevent pep8 warning
        pynn.run(0)
        pynn.end()

    def test_python_interface_merger_friendly(self):
        """tests to set switches of the class"""

        marocco = self.marocco
        user_strat = placer()
        user_strat.m_neuron_block_on_hicann_ordering = placer.merger_tree_friendly
        marocco.neuron_placement.default_placement_strategy(user_strat)

        pynn.setup(marocco=marocco)

        self.run_placement()

        result = self.load_results()

        # the first NB for merger friendly placemnt.
        nb = C.NeuronBlockOnHICANN(C.Enum(3))

        for pop in self.pops:
            for nrn in pop:
                placement_item, = result.placement.find(nrn)
                logical_neuron = placement_item.logical_neuron()
                for denmem in logical_neuron:
                    self.assertEqual(nb, denmem.toNeuronBlockOnHICANN())

    def test_python_interface_nb_enum_decrease(self):
        """tests to set switches of the class"""

        marocco = self.marocco
        user_strat = placer()
        user_strat.m_neuron_block_on_hicann_ordering = placer.neuron_block_on_hicann_enum_decreasing
        marocco.neuron_placement.default_placement_strategy(user_strat)

        pynn.setup(marocco=marocco)

        self.run_placement()

        result = self.load_results()
        nb = C.NeuronBlockOnHICANN(C.Enum(7))
        for pop in self.pops:
            for nrn in pop:
                placement_item, = result.placement.find(nrn)
                logical_neuron = placement_item.logical_neuron()
                for denmem in logical_neuron:
                    self.assertEqual(nb.toEnum(), denmem.toNeuronBlockOnHICANN().toEnum())

    def test_python_interface_nb_enum_increase(self):
        """tests to set switches of the class"""

        marocco = self.marocco
        user_strat = placer()
        user_strat.m_neuron_block_on_hicann_ordering = placer.neuron_block_on_hicann_enum_increasing
        marocco.neuron_placement.default_placement_strategy(user_strat)

        pynn.setup(marocco=marocco)

        self.run_placement()

        result = self.load_results()
        nb = C.NeuronBlockOnHICANN(C.Enum(0))
        for pop in self.pops:
            for nrn in pop:
                placement_item, = result.placement.find(nrn)
                logical_neuron = placement_item.logical_neuron()
                for denmem in logical_neuron:
                    self.assertEqual(nb.toEnum(), denmem.toNeuronBlockOnHICANN().toEnum())

    def test_defaults(self):
        user_strat = placer()
        self.assertEqual(user_strat.m_neuron_block_on_hicann_ordering, user_strat.merger_tree_friendly)
        self.assertEqual(user_strat.m_neuron_block_on_wafer_ordering, user_strat.hicann_on_wafer_then_neuron_block_on_hicann)
        self.assertEqual(user_strat.m_hicann_on_wafer_ordering, user_strat.spiral)
        self.assertEqual(user_strat.m_spiral_center, user_strat.spiral_neighbours)
        self.assertEqual(user_strat.SortPriorityTargets, 1)
        self.assertEqual(user_strat.SortPrioritySources, 1)
        self.assertEqual(user_strat.m_population_placement_priority, user_strat.target_and_source)

    def test_equality(self):
        user_strat = placer()
        default_strat = placer()
        pylogging.set_loglevel(pylogging.get("marocco"),
                               pylogging.LogLevel.TRACE)

        self.assertEqual(user_strat, default_strat)

        user_strat.m_spiral_center = user_strat.spiral_neighbours
        self.assertEqual(user_strat, default_strat)

    def test_clustering(self):
        marocco = self.marocco

        user_strat = placer()
        marocco.neuron_placement.default_placement_strategy(user_strat)

        pynn.setup(marocco=marocco)

        pops = {}
        pops[0] = pynn.Population(1, pynn.IF_cond_exp, {})
        pops[1] = pynn.Population(1, pynn.IF_cond_exp, {})
        pops[2] = pynn.Population(1, pynn.IF_cond_exp, {})

        proj1 = pynn.Projection(pops[0], pops[1], pynn.AllToAllConnector(weights=0.01))
        proj2 = pynn.Projection(pops[1], pops[2], pynn.AllToAllConnector(weights=0.01))

        h = {}
        h[pops[0]] = C.HICANNOnWafer(C.Enum(100))
        # average position is this one, clustering will select it
        h[pops[1]] = C.HICANNOnWafer(C.Enum(101))
        h[pops[2]] = C.HICANNOnWafer(C.Enum(102))
        marocco.manual_placement.on_hicann(pops[0], h[pops[0]])
        marocco.manual_placement.on_hicann(pops[2], h[pops[2]])

        pynn.run(0)
        pynn.end()

        result = self.load_results()
        for key in pops:
            pop = pops[key]
            for nrn in pop:
                placement_item, = result.placement.find(nrn)
                logical_neuron = placement_item.logical_neuron()
                for denmem in logical_neuron:
                    self.assertEqual(h[pop].toEnum(), denmem.toHICANNOnWafer().toEnum())

    def test_cluster_source(self):
        marocco = self.marocco

        user_strat = placer()
        user_strat.m_spiral_center = user_strat.spiral_neighbours_source
        marocco.neuron_placement.default_placement_strategy(user_strat)

        pynn.setup(marocco=marocco)

        pops = {}
        pops[0] = pynn.Population(1, pynn.IF_cond_exp, {})
        pops[1] = pynn.Population(1, pynn.IF_cond_exp, {})
        pops[2] = pynn.Population(1, pynn.IF_cond_exp, {})

        proj1 = pynn.Projection(pops[0], pops[1], pynn.AllToAllConnector(weights=0.01))
        proj2 = pynn.Projection(pops[1], pops[2], pynn.AllToAllConnector(weights=0.01))

        h = {}
        h[pops[0]] = C.HICANNOnWafer(C.Enum(100))
        # average positon of sources
        h[pops[1]] = C.HICANNOnWafer(C.Enum(100))
        h[pops[2]] = C.HICANNOnWafer(C.Enum(102))
        marocco.manual_placement.on_hicann(pops[0], h[pops[0]])
        marocco.manual_placement.on_hicann(pops[2], h[pops[2]])

        pynn.run(0)
        pynn.end()

        result = self.load_results()
        for key in pops:
            pop = pops[key]
            for nrn in pop:
                placement_item, = result.placement.find(nrn)
                logical_neuron = placement_item.logical_neuron()
                for denmem in logical_neuron:
                    self.assertEqual(h[pop].toEnum(), denmem.toHICANNOnWafer().toEnum())

    def test_cluster_target(self):
        marocco = self.marocco

        user_strat = placer()
        user_strat.m_spiral_center = user_strat.spiral_neighbours_target
        marocco.neuron_placement.default_placement_strategy(user_strat)

        pynn.setup(marocco=marocco)

        pops = {}
        pops[0] = pynn.Population(1, pynn.IF_cond_exp, {})
        pops[1] = pynn.Population(1, pynn.IF_cond_exp, {})
        pops[2] = pynn.Population(1, pynn.IF_cond_exp, {})

        proj1 = pynn.Projection(pops[0], pops[1], pynn.AllToAllConnector(weights=0.01))
        proj2 = pynn.Projection(pops[1], pops[2], pynn.AllToAllConnector(weights=0.01))

        h = {}
        h[pops[0]] = C.HICANNOnWafer(C.Enum(100))
        # average positon of target
        h[pops[1]] = C.HICANNOnWafer(C.Enum(102))
        h[pops[2]] = C.HICANNOnWafer(C.Enum(102))
        marocco.manual_placement.on_hicann(pops[0], h[pops[0]])
        marocco.manual_placement.on_hicann(pops[2], h[pops[2]])

        pynn.run(0)
        pynn.end()

        result = self.load_results()
        for key in pops:
            pop = pops[key]
            for nrn in pop:
                placement_item, = result.placement.find(nrn)
                logical_neuron = placement_item.logical_neuron()
                for denmem in logical_neuron:
                    self.assertEqual(h[pop].toEnum(), denmem.toHICANNOnWafer().toEnum())

    def test_vertical(self):
        pylogging.set_loglevel(pylogging.get("marocco"),
                               pylogging.LogLevel.TRACE)

        marocco = self.marocco

        user_strat = placer()
        user_strat.m_hicann_on_wafer_ordering = user_strat.vertical
        user_strat.m_spiral_center = user_strat.spiral_neighbours
        marocco.neuron_placement.default_placement_strategy(user_strat)

        pynn.setup(marocco=marocco)

        pops = {}
        pops[0] = pynn.Population(128, pynn.IF_cond_exp, {})
        pops[1] = pynn.Population(128, pynn.IF_cond_exp, {})
        pops[2] = pynn.Population(128, pynn.IF_cond_exp, {})

        proj1 = pynn.Projection(pops[0], pops[1], pynn.OneToOneConnector(weights=0.01))
        proj2 = pynn.Projection(pops[1], pops[2], pynn.OneToOneConnector(weights=0.01))

        h = {}
        h[pops[0]] = C.HICANNOnWafer(C.Enum(100))
        # the next free hicann (vertical order)
        h[pops[1]] = C.HICANNOnWafer(C.Enum(72))
        h[pops[2]] = C.HICANNOnWafer(C.Enum(48))
        marocco.manual_placement.on_hicann(pops[0], h[pops[0]])

        pynn.run(0)
        pynn.end()

        result = self.load_results()
        for key in pops:
            pop = pops[key]
            for nrn in pop:
                placement_item, = result.placement.find(nrn)
                logical_neuron = placement_item.logical_neuron()
                for denmem in logical_neuron:
                    self.assertEqual(h[pop].toEnum(), denmem.toHICANNOnWafer().toEnum())


if __name__ == '__main__':
    unittest.main()
