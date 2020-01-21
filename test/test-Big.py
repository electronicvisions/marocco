import copy, unittest, random
from ester import Ester
from pymarocco import *
from pyhalco_hicann_v2 import *
from pyhmf import *

class TestBig(unittest.TestCase):

    def setUp(self, backend=PyMarocco.None):
        self.marocco = PyMarocco()
        self.marocco.backend = backend
        self.marocco.neuron_placement.default_neuron_size(2)

    def tearDown(self):
        del self.marocco

    def test_Big(self):
        with Ester() as ester:
            self.big_network()

    def random_chip_defects(self, N=20):
        hicanns = [HICANNGlobal(Enum(ii)) for ii in range(384)]
        random.shuffle(hicanns)
        for h in hicanns[:N]:
            self.marocco.chips.disabled.append(h)

    def big_network(self):
        setup(marocco=self.marocco)

        # mark random HICANNs as defect
        self.random_chip_defects(50)

        numberOfPopulations = 400
        populationSize = 96
        print "network size: %d" % (numberOfPopulations * populationSize)

        pops = [ Population(populationSize, EIF_cond_exp_isfa_ista) for x in
                range(numberOfPopulations) ]

        for idx, pop in enumerate(pops):

            connector = AllToAllConnector(
                    allow_self_connections=True,
                    weights=1.)

            # build ring like network topology
            Projection(pop, pops[(idx+1)%len(pops)], connector, target='excitatory')

            # add poisson stimulus
            source = Population(1, SpikeSourcePoisson, {'rate' : 2})

            Projection(source, pop, connector, target='excitatory')

        run(1)
        end()

        stats = self.marocco.getStats()
        print "python synapse loss: ", stats.getSynapseLoss()

        #for pop in pops:
            #print pop.getSpikes(),
        #print ""

if __name__ == '__main__':
    unittest.main()
