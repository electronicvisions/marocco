import copy, unittest, random
from pymarocco import *
from pyhalbe.Coordinate import *
import pyhmf as pynn

@unittest.skip("ECM disabled (it's not working currently, and we need Jenkins to get BLUE)")
class TestRandom(unittest.TestCase):

    def setUp(self, backend=PyMarocco.None):
        self.marocco = PyMarocco()
        self.marocco.backend = backend
        self.marocco.neuron_placement.default_neuron_size(4)

    def tearDown(self):
        del self.marocco

    def test_Big(self):
        self.random_network()

    def random_network(self):
        pynn.setup(marocco=self.marocco)

        NUM_POPS = random.randint(10, 100)
        POP_SIZE = random.randint(1, 100)
        PROJ_PROB = 0.2

        pops = [ pynn.Population(POP_SIZE, pynn.EIF_cond_exp_isfa_ista) for x in
                range(NUM_POPS) ]

        connector = pynn.AllToAllConnector(
                allow_self_connections=True,
                weights=1.)

        for src in pops:
            for trg in pops:
                target_type = 'inhibitory' if random.random() < 0.2 else 'excitatory'
                if random.random() < PROJ_PROB:
                    pynn.Projection(src, trg, connector, target=target_type)

        pynn.run(1)
        pynn.end()

        stats = self.marocco.getStats()
        print "python synapse loss: ", stats.getSynapseLoss()


if __name__ == '__main__':
    unittest.main()
