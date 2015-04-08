import copy, unittest, random, logging
import numpy as np
#from ester import Ester
from pymarocco import *
from pyhalbe.Coordinate import *
from pyhmf import *

import pylogging, pyhalbe
pyhalbe.Debug.change_loglevel(0)
pylogging.set_loglevel(pylogging.get("marocco"), pylogging.LogLevel.TRACE)

class TestEmptyCrossbar(unittest.TestCase):

    def setUp(self, backend=PyMarocco.None):
        self.marocco = PyMarocco()
        self.marocco.backend = backend
        self.marocco.placement.setDefaultNeuronSize(2)

    def tearDown(self):
        reset() # pynn reset
        del self.marocco

    def test_EmptyCrossbar(self):
        # FIXME: this test currently runs only in ester-breached mode
        #with Ester() as ester:

        self.setUp()
        # clear crossbar. consequently ALL synapses have to be lost
        self.marocco.routing.use_config = True
        self.marocco.routing.cb_clear()

        # map network
        synapses = self.big_network()

        stats = self.marocco.getStats()
        logging.debug("synapse loss: ", stats.getSynapseLoss())
        self.assertEqual(synapses, stats.getSynapses())

        # make sure, all synapses are lost
        self.assertEqual(synapses, stats.getSynapseLoss())

        # and none are set
        self.assertEqual(0, stats.getSynapsesSet())

    def big_network(self):
        setup(marocco=self.marocco)

        synapses = 0
        numberOfPopulations = random.randint(100, 150)
        logging.debug("number of populations: %d" % (numberOfPopulations))
        #print "numPops: %d" % (numberOfPopulations)

        pops = [ Population(random.randint(50, 85), EIF_cond_exp_isfa_ista) for x in
                range(numberOfPopulations) ]

        for idx, pop in enumerate(pops):

            connectorE = FixedProbabilityConnector(
                    p_connect=0.20,
                    allow_self_connections=True,
                    weights=1.)

            connectorI = FixedProbabilityConnector(
                    p_connect=0.10,
                    allow_self_connections=True,
                    weights=1.)

            # build ring like network topology
            proj = Projection(pop, pops[(idx+1)%len(pops)], connectorE, target='excitatory')
            synapses += np.count_nonzero(proj.getWeights())

            proj = Projection(pop, pops[(idx+1)%len(pops)], connectorI, target='inhibitory')
            synapses += np.count_nonzero(proj.getWeights())

            # add poisson stimulus
            source = Population(1, SpikeSourcePoisson, {'rate' : 2})

            proj = Projection(source, pop, connectorE, target='excitatory')
            synapses += np.count_nonzero(proj.getWeights())

        run(100)
        end()

        logging.debug("synapses counted in python: ", synapses)
        return synapses

if __name__ == '__main__':
    unittest.main()