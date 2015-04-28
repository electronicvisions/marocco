import unittest
import pyhmf as pynn
import pymarocco
import pyhalbe
import pylogging

class TestLookupTable(unittest.TestCase):

    def setUp(self, backend=pymarocco.PyMarocco.None):
        pylogging.log_to_cout(pylogging.LogLevel.ERROR)
        self.llog = pylogging.get("test-LookupTable")
        self.marocco = pymarocco.PyMarocco()
        self.marocco.backend = backend
        self.marocco.placement.setDefaultNeuronSize(4)
        self.assertTrue(hasattr(pymarocco.MappingStats, 'bio_id'))
        self.assertTrue(hasattr(pymarocco.MappingStats, 'hw_id'))
        self.assertTrue(hasattr(pymarocco.MappingStats, 'getBioId'))
        self.assertTrue(hasattr(pymarocco.MappingStats, 'getHwId'))

    def tearDown(self):
        del self.marocco


    def test_mini_network(self):
        pynn.setup(marocco=self.marocco)
        numNeurons = 10000
        numPopulations = 42
        numNeuronsPerPopulation = numNeurons/numPopulations
        for p in range(numPopulations):
            pynn.Population(numNeuronsPerPopulation, pynn.IF_cond_exp, {})
        pynn.run(0)

        self.assertEqual(numPopulations*numNeuronsPerPopulation, self.marocco.getStats().getNumNeurons())

        # old-skool (we (!) count it)
        for p in range(numPopulations):
            bio_id = pymarocco.MappingStats.bio_id()
            bio_id.pop = p
            for n in range(numNeuronsPerPopulation):
                bio_id.neuron = n
                for hw_id in self.marocco.getStats().getHwId(bio_id):
                    reverse_bio_id = self.marocco.getStats().getBioId(hw_id)
                    self.assertEqual(reverse_bio_id, bio_id)
                    #print reverse_bio_id.pop, '/', reverse_bio_id.neuron, ' ----> ', hw_id.hicann, hw_id.outb, hw_id.addr
        pynn.reset()
        pynn.end()


    def helper_test_mapping(self, pop):
        bio_id = pymarocco.MappingStats.bio_id()
        p = pop.euter_id()
        bio_id.pop = pop.euter_id()
        for n in range(len(pop)):
            bio_id.neuron = n
            for hw_id in self.marocco.getStats().getHwId(bio_id):
                reverse_bio_id = self.marocco.getStats().getBioId(hw_id)
                self.assertEqual(reverse_bio_id, bio_id)
                #print bio_id.pop, '/', bio_id.neuron, ' ----> ', hw_id.hicann, hw_id.outb, hw_id.addr


    def test_stimulated_network(self):
        con = pynn.FixedProbabilityConnector(p_connect=1.0, weights=0.004)

        pop_size = 100
        ipu_size = 20

        pynn.setup(marocco=self.marocco)
        ipu1 = pynn.Population(ipu_size, pynn.SpikeSourceArray, {'spike_times': []})
        pop1 = pynn.Population(pop_size, pynn.IF_cond_exp, {})
        pro1 = pynn.Projection(ipu1, pop1, con, target='excitatory')

        ipu2 = pynn.Population(ipu_size, pynn.SpikeSourceArray, {'spike_times': []})
        pop2 = pynn.Population(pop_size, pynn.IF_cond_exp, {})
        pro2 = pynn.Projection(ipu2, pop2, con, target='excitatory')

        pynn.run(0)
        all_pops = [pop1, ipu1, pop2, ipu2]
        map(self.helper_test_mapping, all_pops)
        pynn.end()


    def test_stimulatedinterconnected_network(self):
        con = pynn.FixedProbabilityConnector(p_connect=1.0, weights=0.004)

        pop_size = 100
        ipu_size = 20

        pynn.setup(marocco=self.marocco)
        ipu1 = pynn.Population(ipu_size, pynn.SpikeSourceArray, {'spike_times': []})
        pop1 = pynn.Population(pop_size, pynn.IF_cond_exp, {})
        pro1 = pynn.Projection(ipu1, pop1, con, target='excitatory')

        pop2 = pynn.Population(pop_size, pynn.IF_cond_exp, {})
        pro2 = pynn.Projection(pop1, pop2, con, target='excitatory')

        pop3 = pynn.Population(pop_size, pynn.IF_cond_exp, {})
        pro3 = pynn.Projection(pop2, pop3, con, target='excitatory')

        pynn.run(0)
        all_pops = [ipu1, pop1, pop2, pop3]
        map(self.helper_test_mapping, all_pops)
        pynn.end()


if __name__ == '__main__':
    unittest.main()
