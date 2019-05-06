import unittest
import pyhmf as pynn
from pymarocco import PyMarocco
from pymarocco_runtime import PlacePopulationsBase as placer
from pymarocco_runtime import byNeuronBlockEnumAndPopulationID as placer_linear
from pymarocco_runtime import byNeuronBlockEnumAndPopulationIDasc as placer_linear_asc
from pymarocco_runtime import bySmallerNeuronBlockAndPopulationID as placer_smallNB
import pyhalbe.Coordinate as C

import pylogging
import utils


class PlacementAlgorithms(utils.TestWithResults):
    """
    Tests python instantiation of placement algorithms.
    """
    pops = []

    def network(self):
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

    def test_get_strategy(self):
        """tests acquisiton of a default placement strategy"""
        marocco = PyMarocco()
        p = marocco.neuron_placement
        p_strategy = p.default_placement_strategy()
        self.assertTrue(p_strategy is not None)

    @utils.parametrize([placer(),
                        placer_linear(),
                        placer_linear_asc(),
                        placer_smallNB(),
                      ])
    def test_creation(self, strategy):
        marocco = PyMarocco()
        marocco.neuron_placement.default_placement_strategy(strategy)

        pynn.setup(marocco=marocco)

        self.network()

        self.assertTrue(1 == 1)

    def test_hook_modularity_nb(self):
        """tests to override some hooks of the Placement Base class"""
        class myPlacer(placer):
            def initialise(self):
                print("initialise in python,\
                      remove all but NeuronBlock 4 on HICANN 42")

                # use one of the two ways to access the value
                a = self.m_neuron_blocks.access
                b = self.m_neuron_blocks.value()
                assert(a == b)

                b = sorted(b,
                           key=lambda nb: int(nb.
                                              toHICANNOnWafer().
                                              toEnum().
                                              value())
                           )

                c = []
                for nb in b:
                    if nb.toHICANNOnWafer().toEnum().value() == 42:
                        if(nb.toNeuronBlockOnHICANN().toEnum().value() == 4):
                            c.append(nb)

                # smaller values are in the front.
                # the c++ code pops from the back.
                c = sorted(c,
                           key=lambda nb: int(nb.
                                              toNeuronBlockOnHICANN().
                                              toEnum().
                                              value()
                                              ),
                           reverse=False)

                for nb in c:
                    print("will use : {}".format(nb))

                # There are two options to set it again:
                # use access to set a full vector
                self.m_neuron_blocks.access = c

                # or use the value() to access single elements
                for i in range(len(self.m_neuron_blocks.value())):
                    del(self.m_neuron_blocks.value()[0])
                for nb in c:
                    self.m_neuron_blocks.value().append(nb)

            def loop(self):
                print("loop of the custom placer has been called")
                # OPTIONAL do some stuff, shuffle pops, etc.

            def finalise(self):
                print("finalise of the custom placer has been called")
                # OPTIONAL do some stuff, put pops back to original place etc.

        marocco = self.marocco
        user_strat = myPlacer()

        marocco.neuron_placement.default_placement_strategy(user_strat)

        pynn.setup(marocco=marocco)

        self.network()

        result = self.load_results()
        hicann = C.HICANNOnWafer(C.Enum(42))
        nb = C.NeuronBlockOnHICANN(C.Enum(4))
        for pop in self.pops:
            for nrn in pop:
                placement_item, = result.placement.find(nrn)
                logical_neuron = placement_item.logical_neuron()
                for denmem in logical_neuron:
                    self.assertEqual(hicann, denmem.toHICANNOnWafer())
                    self.assertEqual(nb, denmem.toNeuronBlockOnHICANN())

    def test_loop_modularity_nb(self):
        """tests to override the loop hook with NB handling"""
        class myPlacer(placer):
            def initialise(self):
                b = sorted(self.m_neuron_blocks.access,
                           key=lambda nb: int(nb.
                                              toHICANNOnWafer().
                                              toEnum().
                                              value()
                                              )
                           )
                self.m_neuron_blocks.access = b

            def loop(self):
                print("removing the last NB")

                b = self.m_neuron_blocks.access  # use this or the following
                b = self.m_neuron_blocks.value()

                b = sorted(b,
                           key=lambda nb: int(nb.
                                              toHICANNOnWafer().
                                              toEnum().
                                              value()
                                              )
                           )

                c = []
                for i in range(len(b)-1):
                    c.append(b[i])

                # use access to set full vector
                self.m_neuron_blocks.access = c

                # or use the value() to access single elements
                for i in range(len(self.m_neuron_blocks.value())):
                    del(self.m_neuron_blocks.value()[0])
                for nb in c:
                    self.m_neuron_blocks.value().append(nb)

        marocco = self.marocco
        user_strat = myPlacer()

        marocco.neuron_placement.default_placement_strategy(user_strat)

        pynn.setup(marocco=marocco)

        self.network()

        result = self.load_results()
        hicann = C.HICANNOnWafer(C.Enum(42))
        nb = C.NeuronBlockOnHICANN(C.Enum(4))
        for pop in self.pops:
            for nrn in pop:
                placement_item, = result.placement.find(nrn)
                logical_neuron = placement_item.logical_neuron()
                for denmem in logical_neuron:
                    # all pops must be on different NBs
                    self.assertFalse(nb == denmem.toNeuronBlockOnHICANN()
                                     and hicann == denmem.toHICANNOnWafer())
            nb = denmem.toNeuronBlockOnHICANN()
            hicann = denmem.toHICANNOnWafer()

    @unittest.skip("disabled until m_queue can be used in python missing \
                   class registration to python for C++: \
                   `std::vector<NeuronPlacementRequest>` ")
    def test_access_queue(self):
        """use queue handling"""
        class myPlacer(placer):
            def loop(self):
                print("reversing populations")

                # b = self.m_queue.access;  # use this or the following
                print(dir(self.m_queue.value()[0]))
                b = self.m_queue.value()
                print(dir(b))

        marocco = self.marocco
        user_strat = myPlacer()

        marocco.neuron_placement.default_placement_strategy(user_strat)

        pynn.setup(marocco=marocco)

        self.network()

        result = self.load_results()
        hicann = C.HICANNOnWafer(C.Enum(42))
        nb = C.NeuronBlockOnHICANN(C.Enum(4))
        for pop in self.pops:
            for nrn in pop:
                placement_item, = result.placement.find(nrn)
                logical_neuron = placement_item.logical_neuron()
                for denmem in logical_neuron:
                    # all pops shall be on different NBs
                    self.assertFalse(nb == denmem.toNeuronBlockOnHICANN()
                                     and hicann == denmem.toHICANNOnWafer())
            nb = denmem.toNeuronBlockOnHICANN()
            hicann = denmem.toHICANNOnWafer()


if __name__ == '__main__':
    unittest.main()
