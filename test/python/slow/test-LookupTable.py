import os
import shutil
import tempfile
import unittest

from pymarocco.results import Marocco
import pyhmf as pynn
import pylogging
import pymarocco


class TestLookupTable(unittest.TestCase):
    def setUp(self):
        pylogging.reset()
        pylogging.default_config(pylogging.LogLevel.ERROR)
        pylogging.set_loglevel(
            pylogging.get("marocco"), pylogging.LogLevel.INFO)

        self.log = pylogging.get(__name__)
        self.temporary_directory = tempfile.mkdtemp(prefix="marocco-test-")

        self.marocco = pymarocco.PyMarocco()
        self.marocco.backend = pymarocco.PyMarocco.Without
        self.marocco.persist = os.path.join(
            self.temporary_directory, "results.bin")
        self.marocco.neuron_placement.default_neuron_size(4)
        self.marocco.continue_despite_synapse_loss = True
        self.marocco.calib_backend = pymarocco.PyMarocco.CalibBackend.Default
        self.marocco.defects.backend = pymarocco.Defects.Backend.Without

    def tearDown(self):
        shutil.rmtree(self.temporary_directory, ignore_errors=True)
        del self.marocco

    def test_mini_network(self):
        pynn.setup(marocco=self.marocco)
        numNeurons = 1000
        numPopulations = 42
        numNeuronsPerPopulation = numNeurons//numPopulations

        pops = []
        for p in range(numPopulations):
            pops.append(pynn.Population(numNeuronsPerPopulation, pynn.IF_cond_exp, {}))
        pynn.run(0)
        pynn.end()

        self.assertEqual(numPopulations*numNeuronsPerPopulation, self.marocco.getStats().getNumNeurons())

        list(map(self.helper_test_mapping, pops))

    def helper_test_mapping(self, pop):
        results = Marocco.from_file(self.marocco.persist)
        for n in range(len(pop)):
            items = list(results.placement.find(pop[n]))
            if not items:
                self.fail("Neuron {} of population {} not placed".format(
                    n, pop.euter_id()
                ))
            for item in items:
                self.assertEqual(item.neuron_index(), n)
                self.assertEqual(
                    pop.celltype == pynn.SpikeSourceArray,
                    item.logical_neuron().is_external())

    def test_get_denmems(self):
        pop_size = 2

        for neuron_size in [4, 8, 12, 16, 32]:
            self.marocco.neuron_placement.default_neuron_size(neuron_size)

            pynn.setup(marocco=self.marocco)

            target = pynn.Population(pop_size, pynn.IF_cond_exp, {})

            populations = [target]
            for i in range(3):
                p1 = pynn.Population(
                    pop_size,
                    pynn.SpikeSourceArray,
                    {'spike_times': [1.]})
                p2 = pynn.Population(
                    pop_size,
                    pynn.IF_cond_exp,
                    {})
                pynn.Projection(p1, target,
                                pynn.OneToOneConnector(weights=0.004))
                pynn.Projection(p2, target,
                                pynn.OneToOneConnector(weights=0.004))

                populations.append(p2)

            pynn.run(0)
            pynn.end()

            mapstats = self.marocco.getStats()

            results = Marocco.from_file(self.marocco.persist)
            for pop in populations:
                for nrn in range(pop_size):
                    for item in results.placement.find(pop[nrn]):
                        self.assertFalse(item.logical_neuron().is_external())
                        self.assertEqual(neuron_size, item.logical_neuron().size())

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
        list(map(self.helper_test_mapping, all_pops))
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
        list(map(self.helper_test_mapping, all_pops))
        pynn.end()


if __name__ == '__main__':
    unittest.main()
