import unittest

import pyhalbe
import pyredman
import pylogging
from pymarocco import PyMarocco

from pyhalbe.Coordinate import *
from pyhalbe.Coordinate.iter_all import iter_all
from pyhmf import *

import utils

pyhalbe.Debug.change_loglevel(0)
pylogging.set_loglevel(pylogging.get("marocco"), pylogging.LogLevel.TRACE)


class MaroccoFixture(unittest.TestCase):
    def setUp(self):
        super(MaroccoFixture, self).setUp()
        self.marocco = PyMarocco()
        self.marocco.backend = PyMarocco.None
        self.marocco.placement.setDefaultNeuronSize(2)

        setup(marocco=self.marocco)

    def tearDown(self):
        super(MaroccoFixture, self).tearDown()
        reset()

    def stats(self):
        return self.marocco.getStats()


class TrivialNetworkFixture(MaroccoFixture):
    def setUp(self):
        super(TrivialNetworkFixture, self).setUp()

        target = Population(1, EIF_cond_exp_isfa_ista)
        source = Population(1, EIF_cond_exp_isfa_ista)

        self.chip = HICANNGlobal(Enum(3))
        self.marocco.placement.add(target, self.chip)
        self.marocco.placement.add(source, self.chip)

        connector = AllToAllConnector(
            allow_self_connections=True,
            weights=1.)
        Projection(source, target, connector, target='excitatory')

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
        run(10)
        end()

        synapses = self.synapses
        stats = self.stats()
        self.assertEqual(synapses, stats.getSynapses())
        self.assertEqual(synapses, stats.getSynapsesSet())

    @utils.parametrize(resources.keys())
    def test_disable_all(self, name):
        self.inject_disabled_component(
            name, iter_all(resources[name]))

        run(10)
        end()

        # Assert complete loss of synapses
        synapses = self.synapses
        stats = self.stats()
        self.assertEqual(synapses, stats.getSynapses())
        self.assertEqual(synapses, stats.getSynapseLoss())


class TestNeuronDefects(TrivialNetworkFixture):
    def test_disable_all_neurons(self):
        self.inject_disabled_component('neurons', iter_all(NeuronOnHICANN))

        with self.assertRaisesRegexp(RuntimeError, '(no terminals left)'):
            run(10)
            end()

if __name__ == '__main__':
    unittest.main()
