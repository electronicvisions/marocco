#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os
import shutil
import tempfile
import unittest

import pyhmf as pynn
from pyhalbe.Coordinate import *
import pyhalbe.HICANN
nrn_param = pyhalbe.HICANN.neuron_parameter
import debug_config

import pymarocco
from pymarocco.results import Marocco


class IF_multicond_exp(unittest.TestCase):
    def setUp(self):
        self.temporary_directory = tempfile.mkdtemp(prefix="marocco-test-")

    def tearDown(self):
        shutil.rmtree(self.temporary_directory, ignore_errors=True)

    def test_basic(self):
        """
        tests the routing and parameter trafo of a IF_multicond_exp neuron
        with 4 different synaptic input settings.

        For 4 synaptic targets and hardware neuron size 4, the mapping of
        synapse types is as follows:
        Denmem:       | 0 | 1 |
        Synapse Type: |0 1|2 3| (left and right input)
        
        Build a minimal network with 1 neuron and 4 spike sources each
        connecting to different synaptic target on the neuron. Then check that
        the configuration of the synapse driver and synapses is as expected.
        Furthermore, check that the different parameters for e_rev and tau_syn
        are correctly transformed by getting the FG values (qualitatively).
        """
        marocco=pymarocco.PyMarocco()
        marocco.backend = pymarocco.PyMarocco.None
        marocco.calib_backend = pymarocco.PyMarocco.CalibBackend.Default
        marocco.defects.backend = pymarocco.Defects.Backend.None
        marocco.neuron_placement.skip_hicanns_without_neuron_blacklisting(False)

        marocco.neuron_placement.default_neuron_size(4)
        marocco.wafer_cfg = os.path.join(self.temporary_directory, "wafer.bin")
        marocco.persist = os.path.join(self.temporary_directory, "results.bin")
        used_hicann = HICANNGlobal(Enum(0))

        pynn.setup(marocco=marocco)
        p1 = pynn.Population(1, pynn.IF_multicond_exp)
        # we use 4 different time constants and reversal potentials
        p1.set('e_rev', [0.,-10,-80,-100])
        p1.set('tau_syn', [2,3,4,5])

        # place to a certain HICANN to be able to extract config data afterwards
        marocco.manual_placement.on_hicann(p1, used_hicann)

        s1 = pynn.Population(1, pynn.SpikeSourcePoisson, {'rate':5.})
        s2 = pynn.Population(1, pynn.SpikeSourcePoisson, {'rate':5.})
        s3 = pynn.Population(1, pynn.SpikeSourcePoisson, {'rate':5.})
        s4 = pynn.Population(1, pynn.SpikeSourcePoisson, {'rate':5.})

        prj1 = pynn.Projection(s1,p1,pynn.OneToOneConnector(weights=0.01), target="0")
        prj2 = pynn.Projection(s2,p1,pynn.OneToOneConnector(weights=0.01), target="1")
        prj3 = pynn.Projection(s3,p1,pynn.OneToOneConnector(weights=0.01), target="2")
        prj4 = pynn.Projection(s4,p1,pynn.OneToOneConnector(weights=0.01), target="3")

        p1.record()

        pynn.run(1.)

        h = debug_config.load_hicann_cfg(marocco.wafer_cfg, used_hicann)

        # routing config
        active_drivers = []
        driver_c = None
        for driver in iter_all(SynapseDriverOnHICANN):
            drv_cfg = h.synapses[driver]
            if drv_cfg.is_enabled():
                active_drivers.append(drv_cfg)
                driver_c = driver
        assert len(active_drivers) == 1
        act_drv = active_drivers[0]

        # two different synaptic input sides are used on the synapse driver
        syn_input_top = debug_config.get_syn_in_side(act_drv[RowOnSynapseDriver(top)])
        syn_input_bot = debug_config.get_syn_in_side(act_drv[RowOnSynapseDriver(bottom)])
        self.assertNotEqual(syn_input_top, syn_input_bot)

        # assumed column and input side:
        exptected_mapping = [
                (s1,SynapseColumnOnHICANN(0),left),
                (s2,SynapseColumnOnHICANN(0),right),
                (s3,SynapseColumnOnHICANN(1),left),
                (s4,SynapseColumnOnHICANN(1),right)]

        results = Marocco.from_file(marocco.persist)
        for (src,col,side) in exptected_mapping:
            items = list(results.placement.find(src[0]))
            self.assertEqual(1, len(items))
            item = items[0]
            addr = item.address().toL1Address()
            syns = debug_config.find_synapses(h.synapses,driver_c,addr)
            self.assertEqual(1, len(syns))
            syn = syns[0]
            # check synapse column
            self.assertEqual(syn.toSynapseColumnOnHICANN(), col)
            # check synaptic input side
            row_addr = syn.toSynapseRowOnHICANN()
            self.assertEqual(debug_config.get_syn_in_side(act_drv[row_addr.toRowOnSynapseDriver()]), side)


        # FG values
        nrn_left = NeuronOnHICANN(X(0),Y(0))
        nrn_right = NeuronOnHICANN(X(1),Y(0))
        fgs = h.floating_gates

        # e_rev params are montonic decreasing
        E1 = fgs.getNeuron(nrn_left,nrn_param.E_synx)
        E2 = fgs.getNeuron(nrn_left,nrn_param.E_syni)
        E3 = fgs.getNeuron(nrn_right,nrn_param.E_synx)
        E4 = fgs.getNeuron(nrn_right,nrn_param.E_syni)
        E = [E1,E2,E3,E4]
        for k,l in zip(E,E[1:]):
            self.assertGreater(k, l)

        # tau_syn params are montonic increasing
        T1 = fgs.getNeuron(nrn_left,nrn_param.V_syntcx)
        T2 = fgs.getNeuron(nrn_left,nrn_param.V_syntci)
        T3 = fgs.getNeuron(nrn_right,nrn_param.V_syntcx)
        T4 = fgs.getNeuron(nrn_right,nrn_param.V_syntci)
        T = [T1,T2,T3,T4]
        # HICANN V4: The lower the DAC-Value, the higher the time constant
        for k,l in zip(T,T[1:]):
            self.assertGreater(k, l)

if __name__ == '__main__':
    unittest.main()
