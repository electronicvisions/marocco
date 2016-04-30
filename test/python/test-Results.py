import os
import shutil
import tempfile
import unittest

import pyhalbe.Coordinate as C
from pymarocco.results import Marocco
import pyhmf as pynn
import pylogging
import pymarocco

import utils


class TestResults(unittest.TestCase):
    def setUp(self):
        pylogging.reset()
        pylogging.default_config(pylogging.LogLevel.ERROR)
        pylogging.set_loglevel(
            pylogging.get("marocco"), pylogging.LogLevel.INFO)

        self.log = pylogging.get(__name__)
        self.temporary_directory = tempfile.mkdtemp(prefix="marocco-test-")

        self.marocco = pymarocco.PyMarocco()
        self.marocco.backend = pymarocco.PyMarocco.None
        self.marocco.persist = os.path.join(
            self.temporary_directory, "results.bin")

    def tearDown(self):
        shutil.rmtree(self.temporary_directory, ignore_errors=True)
        del self.marocco

    @utils.parametrize([2, 4, 6, 8])
    def test_small_network(self, neuron_size):
        self.marocco.neuron_placement.default_neuron_size(neuron_size)
        pynn.setup(marocco=self.marocco)

        target = pynn.Population(1, pynn.IF_cond_exp, {})
        p1 = pynn.Population(2, pynn.SpikeSourceArray, {'spike_times': [1.]})
        p2 = pynn.Population(5, pynn.IF_cond_exp, {})
        pops = [target, p1, p2]
        proj1 = pynn.Projection(
            p1, target, pynn.AllToAllConnector(weights=0.004))
        proj2 = pynn.Projection(
            p2, target, pynn.AllToAllConnector(weights=0.004))
        projections = [proj1, proj2]

        pynn.run(0)
        pynn.end()

        results = Marocco.from_file(self.marocco.persist)

        self.assertEqual(sum(map(len, pops)), len(list(results.placement)))
        for pop in pops:
            for n in xrange(len(pop)):
                items = list(results.placement.find(pop[n]))
                self.assertEqual(1, len(items))
                item = items[0]

                bio_neuron = item.bio_neuron()
                self.assertEqual(pop.euter_id(), bio_neuron.population())
                self.assertEqual(n, bio_neuron.neuron_index())

                logical_neuron = item.logical_neuron()
                if pop.celltype == pynn.SpikeSourceArray:
                    self.assertTrue(logical_neuron.is_external())
                    self.assertIsNone(item.neuron_block())
                else:
                    self.assertFalse(logical_neuron.is_external())
                    self.assertIsNotNone(item.neuron_block())
                    self.assertEqual(neuron_size, logical_neuron.size())

                # Every placed population should have an address.
                self.assertIsNotNone(item.dnc_merger())
                address = item.address()
                self.assertIsNotNone(address)


if __name__ == '__main__':
    unittest.main()
