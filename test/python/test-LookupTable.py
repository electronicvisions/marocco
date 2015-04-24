import unittest
import pyhmf as pynn
import pymarocco
import pyhalbe
import pylogging

class TestLookupTable(unittest.TestCase):

    def setUp(self, backend=pymarocco.PyMarocco.None):
        pylogging.log_to_cout(pylogging.LogLevel.ERROR)
        self.marocco = pymarocco.PyMarocco()
        self.marocco.backend = backend
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

        for p in range(numPopulations):
            for n in range(numNeuronsPerPopulation):
                bio_id = pymarocco.MappingStats.bio_id()
                bio_id.pop = p
                bio_id.neuron = n
                for hw_id in self.marocco.getStats().getHwId(bio_id):
                    reverse_bio_id = self.marocco.getStats().getBioId(hw_id)
                    self.assertEqual(reverse_bio_id, bio_id)
                    #print reverse_bio_id.pop, '/', reverse_bio_id.neuron, ' ----> ', hw_id.hicann, hw_id.outb, hw_id.addr
        pynn.end()

if __name__ == '__main__':
    unittest.main()
