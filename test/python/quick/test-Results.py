import os
import shutil
import tempfile
import unittest

from pymarocco.results import Marocco
from pysthal.command_line_util import init_logger
import pyhalbe.Coordinate as C
import pyhmf as pynn
import pylogging
import pymarocco

import utils


class TestResults(unittest.TestCase):
    def setUp(self):
        init_logger("INFO", [
            ("marocco", "DEBUG"),
        ])

        self.log = pylogging.get(__name__)
        self.temporary_directory = tempfile.mkdtemp(prefix="marocco-test-")

        self.marocco = pymarocco.PyMarocco()
        self.marocco.backend = pymarocco.PyMarocco.None
        self.marocco.persist = os.path.join(
            self.temporary_directory, "results.bin")

    def tearDown(self):
        shutil.rmtree(self.temporary_directory, ignore_errors=True)
        del self.marocco

    @utils.parametrize([".xml", ".bin", ".xml.gz", ".bin.gz"])
    def test_file_format(self, extension):
        self.marocco.persist = (
            os.path.splitext(self.marocco.persist)[0] + extension
        )
        pynn.setup(marocco=self.marocco)

        target = pynn.Population(1, pynn.IF_cond_exp, {})

        pynn.run(0)
        pynn.end()

        self.assertTrue(os.path.exists(self.marocco.persist))
        results = Marocco.from_file(self.marocco.persist)

        self.assertEqual(1, len(list(results.placement)))

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

        self.assertTrue(os.path.exists(self.marocco.persist))
        results = Marocco.from_file(self.marocco.persist)

        self.assertEqual(sum(map(len, pops)), len(list(results.placement)))
        for pop in pops:
            for n in xrange(len(pop)):
                items = results.placement.find(pop[n])
                self.assertTrue(isinstance(items, list))
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

    @utils.parametrize([1, 2, 3])
    def test_analog_outputs(self, num_recorded_populations):
        """
        Test that analog outputs are correctly assigned and that
        mapping fails if per-HICANN constraints are broken.
        """
        pynn.setup(marocco=self.marocco)
        hicann = C.HICANNOnWafer(C.Enum(210))

        pops = []
        for i in range(num_recorded_populations):
            pop = pynn.Population(1, pynn.IF_cond_exp, {})
            self.marocco.manual_placement.on_hicann(pop, hicann)
            pop.record_v()
            pops.append(pop)

        if num_recorded_populations > 2:
            with self.assertRaises(RuntimeError):
                pynn.run(0)
                pynn.end()
            return

        pynn.run(0)
        pynn.end()

        results = Marocco.from_file(self.marocco.persist)
        placement_item, = results.placement.find(pop[0])

        aouts = list(results.analog_outputs)
        self.assertEqual(num_recorded_populations, len(aouts))

        for pop in pops:
            placement_item, = list(results.placement.find(pop[0]))

            logical_neuron = placement_item.logical_neuron()
            for aout_item in aouts:
                if aout_item.logical_neuron() == logical_neuron:
                    break
            else:
                self.fail("logical neuron not found in analog outputs result")

            aout_item_ = results.analog_outputs.record(logical_neuron)
            self.assertEqual(aout_item.analog_output(), aout_item_.analog_output())

    def test_spike_input(self):
        self.marocco.wafer_cfg = os.path.join(
            self.temporary_directory, "wafer_cfg.bin")
        pynn.setup(marocco=self.marocco)

        target = pynn.Population(1, pynn.IF_cond_exp, {})

        params = [[1., 2., 3.], [4., 3., 2.]]
        sources = [
            pynn.Population(1, pynn.SpikeSourceArray, {'spike_times': times})
            for times in params
        ]
        for source in sources:
            pynn.Projection(
                source, target, pynn.AllToAllConnector(weights=0.004))

        pynn.run(1000.)

        self.assertTrue(os.path.exists(self.marocco.persist))
        results = Marocco.from_file(self.marocco.persist)

        self.assertEqual(0, len(results.spike_times.get(target[0])))
        for spike_times, pop in zip(params, sources):
            self.assertSequenceEqual(
                spike_times, results.spike_times.get(pop[0]))

        spike_times.append(5.)
        results.spike_times.add(pop[0], 5.)
        self.assertSequenceEqual(spike_times, results.spike_times.get(pop[0]))

        spike_times.extend([6., 7.])
        results.spike_times.add(pop[0], [6., 7.])
        self.assertSequenceEqual(spike_times, results.spike_times.get(pop[0]))

        spike_times = [42., 123.]
        results.spike_times.set(pop[0], spike_times)
        self.assertSequenceEqual(spike_times, results.spike_times.get(pop[0]))

        params[-1][:] = []
        results.spike_times.clear(pop[0])
        self.assertEqual(0, len(results.spike_times.get(pop[0])))

        # Check that modifications are reflected in sthal config container

        results.save(self.marocco.persist, overwrite=True)
        self.marocco.skip_mapping = True
        pynn.run(1000.)

        import pysthal
        wafer_cfg = pysthal.Wafer()
        wafer_cfg.load(self.marocco.wafer_cfg)

        for spike_times, pop in zip(params, sources):
            item, = results.placement.find(pop[0])
            address = item.address()
            l1_address = address.toL1Address()
            hicann_cfg = wafer_cfg[address.toHICANNOnWafer()]
            raw_spikes = hicann_cfg.sentSpikes(
                C.GbitLinkOnHICANN(address.toDNCMergerOnHICANN()))
            raw_spikes = raw_spikes[raw_spikes[:, 1] == l1_address.value(), 0]
            self.assertEqual(len(spike_times), len(raw_spikes))

        pynn.end()

if __name__ == '__main__':
    unittest.main()
