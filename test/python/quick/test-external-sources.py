import unittest

import pyhalbe.Coordinate as C
import pyhmf as pynn
import pylogging

import utils


class TestExternalSources(utils.TestWithResults):

    # 1: general place and route
    # 10: multiples placed
    # 100: using two NB/ two SPL1 repeaters
    # 600: using ~10 NB / multiple hicann
    @utils.parametrize([1, 10, 100, 600])
    def test_external_sorces(self, nsources):
        """
            A lot external sources are placed
            no error should be thrown
        """
        pylogging.set_loglevel(
                pylogging.get("marocco"), pylogging.LogLevel.TRACE)
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
        hicann = C.HICANNOnWafer(C.Enum(100))
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


if __name__ == '__main__':
    unittest.main()
