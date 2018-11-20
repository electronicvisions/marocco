import copy, unittest, random
from pymarocco import *
from pyhalbe.Coordinate import *
import pysthal
import pyhmf as pynn

class TestESS(unittest.TestCase):

    def setUp(self, backend=PyMarocco.ESS):
        self.marocco = PyMarocco()
        self.marocco.backend = backend
        self.marocco.calib_backend = PyMarocco.CalibBackend.Default
        self.marocco.hicann_configurator = pysthal.HICANNConfigurator()

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
        if True:
            pynn.setup(marocco=self.marocco)

            # create neuron with v_rest below v_thresh
            source = pynn.Population(1, pynn.EIF_cond_exp_isfa_ista, {
                     'v_rest': -50.,
                     'v_thresh': -60.,
                     'v_reset': -70.6,
                })

            N = 8 # number of target populations

            p = [ pynn.Population(1, pynn.EIF_cond_exp_isfa_ista) for i in range(N) ]

            # place source on HICANN 0
            source_hicann = self.chip(0)
            self.marocco.manual_placement.on_hicann(source, source_hicann)

            # place targets on all HICANNs on same reticle but random neurons
            nrns = self.shuffle(255)
            for ii, pop in enumerate(p):
                hicann = HICANNGlobal(
                            X(int(source_hicann.x())+ii%4),
                            Y(int(source_hicann.y())+ii/4))
                self.marocco.manual_placement.on_hicann(pop, hicann)
                print pop, hicann

            connector = pynn.AllToAllConnector(
                    allow_self_connections=True,
                    weights=1.)

            store = []
            # connect source to targets
            for trg in p:
                proj = pynn.Projection(source, trg, connector, target='excitatory')
                weights = copy.deepcopy(proj.getWeights())
                store.append((proj, weights))

            # start simulation
            pynn.run(10) # in ms
            pynn.end()

            # make sure we have no synapse loss
            self.assertEqual(0, self.marocco.stats.getSynapseLoss())

            # assert weights are the same (at least as long as we don't send be
            # the transformed digital weights)
            for proj, weights in store:
                self.assertEqual(self.marocco.stats.getWeights(proj), weights)

if __name__ == '__main__':
    unittest.main()
