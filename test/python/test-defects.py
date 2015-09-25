import unittest

import pyhalbe
import pyredman
import pylogging
from pymarocco import PyMarocco

from pyhalbe.Coordinate import *
from pyhalbe.Coordinate.iter_all import iter_all
import pyhmf as sim

import utils

pyhalbe.Debug.change_loglevel(0)
pylogging.set_loglevel(pylogging.get("marocco"), pylogging.LogLevel.TRACE)


class MaroccoFixture(unittest.TestCase):
    def setUp(self):
        super(MaroccoFixture, self).setUp()
        self.marocco = PyMarocco()
        self.marocco.backend = PyMarocco.None

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
        self.marocco.placement.add(target, self.chip)
        self.marocco.placement.add(source, self.chip)

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

        with self.assertRaisesRegexp(RuntimeError, '(no terminals left)'):
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
