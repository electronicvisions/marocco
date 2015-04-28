import unittest
import pyhmf as pynn
import pymarocco

class TestIssue1681(unittest.TestCase):
    def setUp(self, backend=pymarocco.PyMarocco.None):
        self.marocco = pymarocco.PyMarocco()
        self.marocco.backend = backend

    def test_issue1681(self):
        con = pynn.FixedProbabilityConnector(p_connect=1.0, weights=0.004)

        pynn.setup(marocco=self.marocco)
        pop1 = pynn.Population(10, pynn.IF_cond_exp, {})
        ipu1 = pynn.Population(1, pynn.SpikeSourceArray, {'spike_times': []})
        pro1 = pynn.Projection(ipu1, pop1, con, target='excitatory')

        # Issue 1681 "IndexError after mapping ends" is triggered by adding a
        # population after a projection:
        # Expected Output: NO ERROR
        # Actual Output: "IndexError: vector::_M_range_check" (i.e. abnormal termination)
        pop2 = pynn.Population(10, pynn.IF_cond_exp, {})
        pynn.run(0)

if __name__ == '__main__':
    unittest.main()
