import unittest

import pyhalbe
import pyredman
import pylogging
from pymarocco import PyMarocco, Defects

from pyhalbe.Coordinate import *
import pyhmf as sim

import utils

pyhalbe.Debug.change_loglevel(0)
pylogging.set_loglevel(pylogging.get("marocco"), pylogging.LogLevel.INFO)


class MaroccoFixture(unittest.TestCase):
    def setUp(self):
        super(MaroccoFixture, self).setUp()
        self.marocco = PyMarocco()
        self.marocco.backend = PyMarocco.None
        self.marocco.calib_backend = PyMarocco.CalibBackend.Default
        self.marocco.defects.backend = Defects.Backend.None
        self.marocco.merger_routing.strategy(self.marocco.merger_routing.minimize_number_of_sending_repeaters)

        sim.setup(marocco=self.marocco)

    def tearDown(self):
        super(MaroccoFixture, self).tearDown()
        # sim.reset() only needed for multiple run() in the same session
        sim.end()

    def stats(self):
        return self.marocco.getStats()


class TrivialNetworkFixture(MaroccoFixture):
    def setUp(self):
        super(TrivialNetworkFixture, self).setUp()

        target = sim.Population(1, sim.EIF_cond_exp_isfa_ista)
        source = sim.Population(1, sim.EIF_cond_exp_isfa_ista)

        self.chip = HICANNGlobal(Enum(3))
        self.marocco.manual_placement.on_hicann(target, self.chip)
        self.marocco.manual_placement.on_hicann(source, self.chip)
        self.marocco.continue_despite_synapse_loss = True

        connector = sim.AllToAllConnector(
            allow_self_connections=True,
            weights=1.)
        sim.Projection(source, target, connector, target='excitatory')

        self.synapses = 1

    def inject_disabled_component(self, attr, it):
        hicann = pyredman.Hicann()

        for component in it:
            getattr(hicann, attr)().disable(component)

        # Note: Overwrites values set by prior calls
        self.marocco.defects.inject(self.chip, hicann)


resources = {
    'synapses': SynapseOnHICANN,
    'drivers': SynapseDriverOnHICANN,
    'hbuses': HLineOnHICANN,
    'vbuses': VLineOnHICANN
}


class TestSynapseLoss(TrivialNetworkFixture):
    def test_no_disabled_synapses(self):
        sim.run(10)

        synapses = self.synapses
        stats = self.stats()
        self.assertEqual(synapses, stats.getSynapses())
        self.assertEqual(synapses, stats.getSynapsesSet())

    @utils.parametrize(resources.keys())
    def test_disable_all(self, name):
        self.inject_disabled_component(
            name, iter_all(resources[name]))

        sim.run(10)

        # Assert complete loss of synapses
        synapses = self.synapses
        stats = self.stats()
        self.assertEqual(synapses, stats.getSynapses())
        self.assertEqual(synapses, stats.getSynapseLoss())


class TestNeuronDefects(TrivialNetworkFixture):
    def test_disable_all_neurons(self):
        self.inject_disabled_component('neurons', iter_all(NeuronOnHICANN))

        with self.assertRaisesRegexp(RuntimeError, 'unable to implement manual placement request'):
            sim.run(10)

merger_resources = {
    'dncmergers': DNCMergerOnHICANN,
    'mergers0': Merger0OnHICANN,
    'mergers1': Merger1OnHICANN,
    'mergers2': Merger2OnHICANN,
    'mergers3': Merger3OnHICANN,
}

class TestMergerDefects(TrivialNetworkFixture):
    """
    Tests that the mapping failes, if all merger are marked as defect.
    """
    @utils.parametrize(merger_resources.keys())
    def test_disable_all(self, name):
        self.inject_disabled_component(
            name, iter_all(merger_resources[name]))

        with self.assertRaisesRegexp(RuntimeError,
                                     '(unroutable mergers)'):
            sim.run(10)

if __name__ == '__main__':
    unittest.main()
