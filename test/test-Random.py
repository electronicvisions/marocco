import copy, unittest, random
from ester import Ester
from pymarocco import *
from pyhalbe.Coordinate import *
from pyhmf import *

class TestRandom(unittest.TestCase):

    def setUp(self, backend=PyMarocco.None):
        self.marocco = PyMarocco()
        self.marocco.backend = backend
        self.marocco.neuron_placement.default_neuron_size(4)

    def tearDown(self):
        del self.marocco

    def test_Big(self):
        with Ester() as ester:
            self.random_network(ester)

    def random_network(self, ester):
        setup(marocco=self.marocco)

        NUM_POPS = random.randint(10, 1000)
        POP_SIZE = random.randint(1, 100)
        PROJ_PROB = 0.2

        pops = [ Population(POP_SIZE, EIF_cond_exp_isfa_ista) for x in
                range(NUM_POPS) ]

        connector = AllToAllConnector(
                allow_self_connections=True,
                weights=1.)

        for src in pops:
            for trg in pops:
                target_type = 'inhibitory' if random.random() < 0.2 else 'excitatory'
                if random.random() < PROJ_PROB:
                    Projection(src, trg, connector, target=target_type)

        run(1)
        end()

        stats = self.marocco.getStats()
        print "python synapse loss: ", stats.getSynapseLoss()

        self.assertEqual(None, ester.exit_code())


if __name__ == '__main__':
    unittest.main()
