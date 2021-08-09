import unittest

from pyhalco_common import Enum
import pyhalco_hicann_v2 as C
import pyhmf as pynn
import pylogging
import itertools

import utils


class TestExternalSources(utils.TestWithResults):

    # 1: general place and route
    # 10: multiples placed
    # 100: using two NB/ two SPL1 repeaters
    # 600: using ~10 NB / multiple hicann
    # test for exceeded rate
    @utils.parametrize([1, 10, 100, 600])
    def test_external_sources_rates(self, nsources):
        """
            A lot external sources are placed
            no error should be thrown
        """
        pylogging.set_loglevel(
                pylogging.get("marocco"), pylogging.LogLevel.DEBUG)
        pylogging.set_loglevel(
                pylogging.get("Calibtic"), pylogging.LogLevel.ERROR)

        pynn.setup(marocco=self.marocco)
        self.marocco.neuron_placement.default_neuron_size(4)

        # ignore the synapse driver chain length.
        self.marocco.synapse_routing.driver_chain_length(100)

        # we expect synapse loss, but we dont care.
        # we want this tests not to throw exceptions.
        self.marocco.continue_despite_synapse_loss = True

        target = pynn.Population(1, pynn.IF_cond_exp, {})
        hicann = C.HICANNOnWafer(Enum(100))
        self.marocco.manual_placement.on_hicann(target, hicann)

        exsource = pynn.Population(nsources,
                                   pynn.SpikeSourcePoisson,
                                   {'rate': 1.})
        proj = pynn.Projection(exsource,
                               target,
                               pynn.AllToAllConnector(weights=1.))

        # access to proj so flake8 keeps silent
        proj.size

        pynn.run(0)
        pynn.end()

        results = self.load_results()
        synapses = results.synapse_routing.synapses()

        self.assertEqual(nsources, synapses.size())


    # 1: general place and route
    # 10: multiples placed
    # 30: would fit into one NB/ SPL1 repeater,
    #     but should be splitted, because of chain length
    # 100: using two NB/ two SPL1 repeaters
    #
    # 600: using ~10 NB / multiple hicann
    # test without rate limit
    @utils.parametrize([1, 10, 30, 100, 600])
    def test_external_sources_drivers(self, nsources):
        """
            A lot external sources are placed
            no error should be thrown
            the sources should be split
        """
        pylogging.set_loglevel(
                pylogging.get("marocco"), pylogging.LogLevel.DEBUG)
        pylogging.set_loglevel(
                pylogging.get("Calibtic"), pylogging.LogLevel.ERROR)

        pynn.setup(marocco=self.marocco)
        self.marocco.neuron_placement.default_neuron_size(4)

        # ensure a limited synapse driver chain length.
        self.marocco.synapse_routing.driver_chain_length(3)

        # if synapse loss occours we want to handle it on our own
        self.marocco.continue_despite_synapse_loss = True

        target = pynn.Population(1, pynn.IF_cond_exp, {})
        hicann = C.HICANNOnWafer(Enum(100))
        self.marocco.manual_placement.on_hicann(target, hicann)

        exsource = pynn.Population(nsources,
                                   pynn.SpikeSourcePoisson,
                                   {'rate': 1.})
        proj = pynn.Projection(exsource,
                               target,
                               pynn.AllToAllConnector(weights=1.))

        # access to proj so flake8 keeps silent
        proj.size

        pynn.run(0)
        pynn.end()

        results = self.load_results()
        synapses = results.synapse_routing.synapses()
        placement = results.placement;

        # test for synapse loss
        self.assertEqual(nsources, synapses.size())


        for dnc in C.iter_all(C.DNCMergerOnWafer):
            PonDNC = placement.find(dnc) # PopulationOnDNC
            if PonDNC:
                ## with a neuron size of 4 and  a chain length of 3,
                ## around 12 sources can fit into a merger
                self.assertTrue(len(PonDNC) <= 12)

    # Projections:
    # 1: general place and route
    # 2: multiple projections,
    # 20: this ammount should require more than 3 synapse drivers
    #     but the size is 1, so it cant be split. the user has to live with that
    # sources:
    # 1: place the source, none theless it cant be split
    # 30: place the sources on multiple DNC,
    #
    # test sources with multiple projections
    @utils.parametrize(itertools.product( [1, 2, 20],[1,30]) )
    def test_external_sources_projections(self, params ):
        nprojections = params[0]
        nsources = params[1]
        print((nprojections, nsources))
        """
            An external sources has multiple projections
            so it should be split if it wuld not be of size 1
            so unfortunately the users would need to live with that.
        """
        pylogging.set_loglevel(
                pylogging.get("marocco"), pylogging.LogLevel.TRACE)
        pylogging.set_loglevel(
                pylogging.get("Calibtic"), pylogging.LogLevel.ERROR)

        pynn.setup(marocco=self.marocco)
        self.marocco.neuron_placement.default_neuron_size(4)

        # ensure a limited synapse driver chain length.
        self.marocco.synapse_routing.driver_chain_length(3)

        # we expect synapse loss, but we dont care, as the source cant be split.
        # we want this tests not to throw exceptions.
        self.marocco.continue_despite_synapse_loss = True

        target = pynn.Population(1, pynn.IF_cond_exp, {})
        hicann = C.HICANNOnWafer(Enum(100))
        self.marocco.manual_placement.on_hicann(target, hicann)

        exsource = pynn.Population(nsources,
                                   pynn.SpikeSourcePoisson,
                                   {'rate': 1.})
        for i in range(nprojections):
            proj = pynn.Projection(exsource,
                                   target,
                                   pynn.AllToAllConnector(weights=1.))

        # access to proj so flake8 keeps silent
        proj.size

        pynn.run(0)
        pynn.end()

        results = self.load_results()
        synapses = results.synapse_routing.synapses()
        placement = results.placement;

        for dnc in C.iter_all(C.DNCMergerOnWafer):
            PonDNC = placement.find(dnc) # PopulationOnDNC
            if PonDNC:
                ## if driver requirements exceeded, only one source should be
                ## placed on the DNC, but synapse loss is still expected
                if(nprojections > 4): # this number is just guessed
                    self.assertTrue(len(PonDNC) <= 1)
                else:
                    self.assertTrue(len(PonDNC) <= 12)


if __name__ == '__main__':
    unittest.main()
