import copy, unittest, random
from ester import Ester
from pymarocco import *
from pyhalbe.Coordinate import *
from pyhmf import *

class TestESS(unittest.TestCase):

    def setUp(self, backend=PyMarocco.ESS):
        self.marocco = PyMarocco()
        self.marocco.backend = backend

    def tearDown(self):
        del self.marocco

    @staticmethod
    def shuffle(a, *args):
        r = range(a, *args)
        random.shuffle(r)
        return r

    def chip(self, hicann):
        return HICANNGlobal(Enum(hicann))

    def test_TwoNeuron(self):
        with Ester() as ester:
            setup(marocco=self.marocco)

            # create neuron with v_rest below v_thresh
            source = Population(1, EIF_cond_exp_isfa_ista, {
                     'v_rest': -50.,
                     'v_thresh': -60.,
                     'v_reset': -70.6,
                })

            N = 8 # number of target populations

            p = [ Population(1, EIF_cond_exp_isfa_ista) for i in range(N) ]

            # place source on HICANN 0
            source_hicann = self.chip(0)
            self.marocco.placement.add(source, source_hicann)

            # place targets on all HICANNs on same reticle but random neurons
            nrns = self.shuffle(255)
            for ii, pop in enumerate(p):
                hicann = HICANNGlobal(
                            X(int(source_hicann.x())+ii%4),
                            Y(int(source_hicann.y())+ii/4))
                self.marocco.placement.add(pop, hicann)
                print pop, hicann

            connector = AllToAllConnector(
                    allow_self_connections=True,
                    weights=1.)

            store = []
            # connect source to targets
            for trg in p:
                proj = Projection(source, trg, connector, target='excitatory')
                weights = copy.deepcopy(proj.getWeights())
                store.append((proj, weights))

            # start simulation
            run(10) # in ms
            end()

            # make sure we have no synapse loss
            self.assertEqual(0, self.marocco.stats.getSynapseLoss())

            # assert weights are the same (at least as long as we don't send be
            # the transformed digital weights)
            for proj, weights in store:
                self.assertEqual(self.marocco.stats.getWeights(proj), weights)

if __name__ == '__main__':
    unittest.main()
