import copy, unittest, random, logging
import numpy as np
#from ester import Ester
from pymarocco import *
from pyhalbe.Coordinate import *
import pyhmf as pynn
import pyredman

import utils

import pylogging, pyhalbe
pyhalbe.Debug.change_loglevel(0)
pylogging.set_loglevel(pylogging.get("marocco"), pylogging.LogLevel.INFO)

class TestSynapseLoss(unittest.TestCase):

    def setUp(self, backend=PyMarocco.None):
        self.marocco = PyMarocco()
        self.marocco.backend = backend
        self.marocco.neuron_placement.default_neuron_size(2)
        self.marocco.neuron_placement.skip_hicanns_without_neuron_blacklisting(False)
        self.marocco.continue_despite_synapse_loss = True
        self.marocco.synapse_routing.driver_chain_length(SynapseDriverOnQuadrant.size)
        self.marocco.calib_backend = PyMarocco.CalibBackend.Default
        self.marocco.defects.backend = Defects.Backend.None

    def tearDown(self):
        pynn.reset() # pynn reset
        del self.marocco

    def test_Normal(self):
        # FIXME: this test currently runs only in ester-breached mode
        #with Ester() as ester:

        # map network
        synapses = self.big_network()

        stats = self.marocco.getStats()
        logging.debug("synapse loss: {}".format(stats.getSynapseLoss()))
        self.assertEqual(synapses, stats.getSynapses())

        # at least one synapse should be released, come on...
        self.assertLess(stats.getSynapseLoss(), synapses)

        # finally assert, that loss and weights set sum up to total number of synapses
        self.assertEqual(synapses, stats.getSynapseLoss()+stats.getSynapsesSet())

    def big_network(self):
        pynn.setup(marocco=self.marocco)

        synapses = 0
        numberOfPopulations = random.randint(100, 150)
        logging.debug("number of populations: %d" % (numberOfPopulations))
        #print "numPops: %d" % (numberOfPopulations)

        # FIXME: spuouriously fails, why?
        #pops = [ Population(random.randint(50, 85), EIF_cond_exp_isfa_ista) for x in
                #range(numberOfPopulations) ]
        pops = [ pynn.Population(80, pynn.EIF_cond_exp_isfa_ista) for x in
                range(numberOfPopulations) ]

        for idx, pop in enumerate(pops):

            connectorE = pynn.FixedProbabilityConnector(
                    p_connect=0.20,
                    allow_self_connections=True,
                    weights=1.)

            connectorI = pynn.FixedProbabilityConnector(
                    p_connect=0.10,
                    allow_self_connections=True,
                    weights=1.)

            # build ring like network topology
            proj = pynn.Projection(pop, pops[(idx+1)%len(pops)], connectorE, target='excitatory')
            synapses += np.count_nonzero(proj.getWeights())

            proj = pynn.Projection(pop, pops[(idx+1)%len(pops)], connectorI, target='inhibitory')
            synapses += np.count_nonzero(proj.getWeights())

            # add poisson stimulus
            source = pynn.Population(1, pynn.SpikeSourcePoisson, {'rate' : 2})

            proj = pynn.Projection(source, pop, connectorE, target='excitatory')
            synapses += np.count_nonzero(proj.getWeights())

        pynn.run(100)
        pynn.end()

        logging.debug("synapses counted in python: %d", synapses)
        return synapses

    @utils.parametrize(["PopulationView", "Population", "Assembly"])
    def test_loss_in_wafer_routing(self, mode):
        h0 = HICANNGlobal(Enum(0))
        h1 = HICANNGlobal(Enum(1))

        # disable all horizontal buses on h0
        hicann = pyredman.Hicann()
        for hbus in iter_all(HLineOnHICANN):
            hicann.hbuses().disable(hbus)
        self.marocco.defects.inject(h0, hicann)

        pynn.setup(marocco=self.marocco)
        n1 = 100
        n2 = 100
        p1 = pynn.Population(n1, pynn.EIF_cond_exp_isfa_ista)
        p2 = pynn.Population(n2, pynn.EIF_cond_exp_isfa_ista)

        self.marocco.manual_placement.on_hicann(p1, h0)
        self.marocco.manual_placement.on_hicann(p2, h1)

        n_post = 10
        if mode == "Population":
            src = p1
            tgt = p2
            exp_loss = len(src)*n_post
        elif mode == "PopulationView":
            src = p1[n1/2:n1]
            tgt = p2[n2/2:n2]
            exp_loss = len(src)*n_post
        elif mode == "Assembly":
            src = pynn.Assembly(p1,p2)
            tgt = p2
            exp_loss = len(p1)*n_post

        conn = pynn.FixedNumberPostConnector(n_post,
                allow_self_connections=True, weights=1.)
        proj = pynn.Projection(src, tgt, conn, target='excitatory')

        pynn.run(100)
        pynn.end()

        # check stats
        self.assertEqual(exp_loss,
                self.marocco.stats.getSynapseLossAfterL1Routing())
        self.assertEqual(exp_loss,
                self.marocco.stats.getSynapseLoss())

        # check weight matrices
        orig_weights = proj.getWeights(format="array")
        mapped_weights = self.marocco.stats.getWeights(proj)
        lost_syns = np.logical_and(np.isfinite(orig_weights), np.isnan(mapped_weights))
        self.assertEqual(exp_loss, np.count_nonzero(lost_syns))


if __name__ == '__main__':
    unittest.main()
