#!/usr/bin/python

import os
import shutil
import tempfile
import unittest

import pylogging
import pyhmf as pynn
import pyhalbe
import pyhalco_common
import pyhalco_hicann_v2 as C
import numpy as np
import debug_config

import pymarocco
from pymarocco.results import Marocco


pylogging.reset()
pylogging.default_config(pylogging.LogLevel.INFO)
pylogging.set_loglevel(pylogging.get("marocco"), pylogging.LogLevel.DEBUG)

def default_marocco():
    marocco = pymarocco.PyMarocco()
    marocco.neuron_placement.default_neuron_size(4)
    marocco.synapse_routing.driver_chain_length(C.SynapseDriverOnQuadrant.size)
    marocco.experiment.speedup(10000.)
    marocco.defects.backend = pymarocco.Defects.Backend.Without

    return marocco

class TestRateDependentInputPlacement(unittest.TestCase):
    """Test input placement with consider_firing_rate=True for diverse settings"""

    hicann_bw = 17.8e6
    fpga_bw = 125.e6

    def setUp(self):
        self.temporary_directory = tempfile.mkdtemp(prefix="marocco-test-")

    def tearDown(self):
        shutil.rmtree(self.temporary_directory, ignore_errors=True)

    def run_experiment(self, marocco, n_stim, rate, poisson=True, shuffle=False):
        """
        runs experiment with `n_stim` SpikeSources, firing at
        `rate` Hz, all connected to 1 neuron.
        returns a  result dictionary, with keys `hicanns` and `fpgas`, each
        containing used FPGA/HICANN coords and number of sources mapped per
        FPGA/HICANN.
        further params:
        poisson - if True, use SpikeSourcePoisson, else use SpikeSourceArrays
                  with regular firing
        shuffle - if True, the spike times used for SpikeSourceArray are
                  shuffled, i.e. they are not sorted. Only valid if
                  poisson=True)
        """

        sim_duration = 200.
        marocco.persist = os.path.join(self.temporary_directory, "results.bin")
        pynn.setup(marocco=marocco)

        exc_pop = pynn.Population(1, pynn.IF_cond_exp, {})

        # place target onto a hicann in the center of reticle and at the border of the wafer
        # such that hicanns from the same reticle are used with preference (1 reticle -> same fpga)
        marocco.manual_placement.on_hicann(
            exc_pop, C.HICANNOnWafer(pyhalco_common.Enum(1)))

        if poisson:
            pop_stim = pynn.Population(n_stim, pynn.SpikeSourcePoisson, {'rate':rate, 'duration':sim_duration})
        else:
            pop_stim = pynn.Population(n_stim, pynn.SpikeSourceArray)
            for i in range(n_stim):
                isi = 1.e3/rate
                spike_times = np.arange( (i+1)*1./n_stim*isi, sim_duration, isi)
                if shuffle:
                    np.random.shuffle(spike_times)
                pop_stim[i:i+1].set('spike_times', spike_times.tolist())
        a2a = pynn.AllToAllConnector(weights=0.001, delays=2.)
        pynn.Projection( pop_stim, exc_pop, a2a, target='excitatory')
        pynn.run(sim_duration)

        results = Marocco.from_file(marocco.persist)
        hicanns = {} # count number of stimuli mapped on Hicann
        fpgas = {} # count number of stimuli mapped on fpga
        for idx in range(len(pop_stim)):
            items = list(results.placement.find(pop_stim[idx]))
            # stim nrns are only placed once per wafer
            self.assertEqual(1, len(items))
            address = items[0].address()
            hicann_str = str(address.toHICANNOnWafer())
            hicanns[hicann_str] = hicanns.get(hicann_str, 0) + 1
            hicann_global = C.HICANNGlobal(
                address.toHICANNOnWafer(), C.Wafer())
            fpga_str = str(hicann_global.toFPGAGlobal())
            fpgas[fpga_str] = fpgas.get(fpga_str, 0) + 1

        pynn.end()
        return dict(hicanns=hicanns,fpgas=fpgas)

    def test_hicann_limit(self):
        """total rate exceeds BW of 1 HICANN -> 2 HICANNs are used"""
        marocco = default_marocco()
        marocco.input_placement.consider_firing_rate(True)
        marocco.input_placement.bandwidth_utilization(1.0)

        total_rate = 1.05*self.hicann_bw / marocco.experiment.speedup()
        poisson_rate = 100.
        n_stim = int(np.ceil(total_rate/poisson_rate))
        r = self.run_experiment(marocco, n_stim, poisson_rate)

        hicanns = r['hicanns']
        # For the total rate 2 HICANNs are needed
        self.assertEqual(2, len(hicanns))

        max_stim_per_hicann = int(self.hicann_bw / marocco.experiment.speedup() / poisson_rate)
        # max_stim_per_hicann fit onto the first hicanns, the rest should be on other hicann
        self.assertEqual(set(hicanns.values()), set((max_stim_per_hicann, n_stim-max_stim_per_hicann)))

    def test_fpga_limit(self):
        """total rate exceeds BW of 1 FPGA -> 2 FPGAs are used"""
        marocco = default_marocco()
        marocco.input_placement.consider_firing_rate(True)
        marocco.input_placement.bandwidth_utilization(1.0)

        poisson_rate = 100.
        total_rate = 1.05*self.fpga_bw / marocco.experiment.speedup()
        n_stim = int(np.ceil(total_rate/poisson_rate))
        r = self.run_experiment(marocco, n_stim, poisson_rate)

        fpgas = r['fpgas']
        self.assertEqual(2, len(fpgas))

    def test_speedup(self):
        """
        same experiment as test_hicann_limit, but decrease speedup to 5000., so
        that everything fits onto 1 hicann.
        """

        marocco = default_marocco()
        marocco.input_placement.consider_firing_rate(True)
        marocco.input_placement.bandwidth_utilization(1.0)

        total_rate = 1.05*self.hicann_bw / marocco.experiment.speedup()
        poisson_rate = 100.
        n_stim = int(np.ceil(total_rate/poisson_rate))

        # reduce speedup
        marocco.experiment.speedup(5000.)
        r = self.run_experiment(marocco, n_stim, poisson_rate)

        # with reduced speedup, 1 hicann is sufficient
        self.assertEqual(1, len(r['hicanns']))

    def test_utilization(self):
        """
        same experiment as test_hicann_limit, but decrease utilization to 0.5 so
        that 3 hicanns are needed
        """

        marocco = default_marocco()
        marocco.input_placement.consider_firing_rate(True)
        marocco.input_placement.bandwidth_utilization(0.5)

        total_rate = 1.05*self.hicann_bw / marocco.experiment.speedup()
        poisson_rate = 100.
        n_stim = int(np.ceil(total_rate/poisson_rate))

        r = self.run_experiment(marocco, n_stim, poisson_rate)

        # with utilization=0.5, even 2 hicanns are not sufficient
        self.assertEqual(3, len(r['hicanns']))

    def test_spike_source_array(self):
        """test hicann limit with spike source array.
        total rate exceeds BW of 1 HICANN -> 2 HICANNs are used"""
        marocco = default_marocco()
        marocco.input_placement.consider_firing_rate(True)
        marocco.input_placement.bandwidth_utilization(1.0)

        total_rate = 1.05*self.hicann_bw / marocco.experiment.speedup()
        rate = 100.
        n_stim = int(np.ceil(total_rate/rate))
        r = self.run_experiment(marocco, n_stim, rate, poisson=False)

        hicanns = r['hicanns']
        # For the total rate 2 HICANNs are needed
        self.assertEqual(2, len(hicanns))

        max_stim_per_hicann = int(self.hicann_bw / marocco.experiment.speedup() / rate)
        # max_stim_per_hicann fit onto the first hicanns, the rest should be on other hicann
        self.assertEqual(set(hicanns.values()), set((max_stim_per_hicann, n_stim-max_stim_per_hicann)))

    def test_spike_source_array_shuffled(self):
        """same as test_spike_source_array, but with randomly shuffled spike
        times."""
        marocco = default_marocco()
        marocco.input_placement.consider_firing_rate(True)
        marocco.input_placement.bandwidth_utilization(1.0)

        total_rate = 1.05*self.hicann_bw / marocco.experiment.speedup()
        rate = 100.
        n_stim = int(np.ceil(total_rate/rate))
        r = self.run_experiment(marocco, n_stim, rate, poisson=False,
                shuffle=True)

        hicanns = r['hicanns']
        # For the total rate 2 HICANNs are needed
        self.assertEqual(2, len(hicanns))

        max_stim_per_hicann = int(self.hicann_bw / marocco.experiment.speedup() / rate)
        # max_stim_per_hicann fit onto the first hicanns, the rest should be on other hicann
        self.assertEqual(set(hicanns.values()), set((max_stim_per_hicann, n_stim-max_stim_per_hicann)))


if __name__ == '__main__':
    unittest.main()
