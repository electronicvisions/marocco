import unittest
import pyhmf as pynn
import pymarocco

class TestIssue1565(unittest.TestCase):
    def setUp(self, backend=pymarocco.PyMarocco.Without):
        self.marocco = pymarocco.PyMarocco()
        self.marocco.backend = backend

    def tearDown(self):
        del self.marocco

    def test_issue1565(self):
        # although there is only 1 synapse column per neuron (of size 2), a 2nd synapse is used
        self.marocco.neuron_placement.default_neuron_size(2)
        con = pynn.FixedProbabilityConnector(p_connect=1.0, weights=0.004)

        pynn.setup(marocco=self.marocco)
        pop1 = pynn.Population(10, pynn.IF_cond_exp, {})
        ipu1 = pynn.Population(2, pynn.SpikeSourceArray, {'spike_times': []})
        pro1 = pynn.Projection(ipu1, pop1, con, target='excitatory')
        pynn.run(0)

if __name__ == '__main__':
    unittest.main()
