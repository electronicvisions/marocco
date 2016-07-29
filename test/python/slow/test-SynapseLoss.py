import copy, unittest, random, logging
import numpy as np
#from ester import Ester
from pymarocco import *
from pyhalbe.Coordinate import *
import pyhmf as pynn

import pylogging, pyhalbe
pyhalbe.Debug.change_loglevel(0)
pylogging.set_loglevel(pylogging.get("marocco"), pylogging.LogLevel.TRACE)

class TestSynapseLoss(unittest.TestCase):

    def setUp(self, backend=PyMarocco.None):
        self.marocco = PyMarocco()
        self.marocco.backend = backend
        self.marocco.neuron_placement.default_neuron_size(2)

    def tearDown(self):
        pynn.reset() # pynn reset
        del self.marocco

    def test_Normal(self):
        # FIXME: this test currently runs only in ester-breached mode
        #with Ester() as ester:

        self.marocco.routing.use_config = True
        self.marocco.routing.cb_reset()

        # map network
        synapses = self.big_network()

        stats = self.marocco.getStats()
        logging.debug("synapse loss: ", stats.getSynapseLoss())
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

        logging.debug("synapses counted in python: ", synapses)
        return synapses

if __name__ == '__main__':
    unittest.main()
