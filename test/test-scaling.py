import os, sys, unittest, tempfile, logging
import numpy as np
import scripts.scaling as scaling
from ester import Ester
from pymarocco import PyMarocco

def generate_network_scaling(num_pops, pop_size, results):

    class ScalingBase(unittest.TestCase):
        DEFAULT_NEURON_SIZE = 2

        def setUp(self, backend=PyMarocco.None):
            self.marocco = PyMarocco()
            self.marocco.backend = backend
            self.marocco.placement.setDefaultNeuronSize(
                    ScalingBase.DEFAULT_NEURON_SIZE)

        def tearDown(self):
            del self.marocco

        def test_network(self):
            with Ester() as ester:
                self.assertTrue(ester.running())

                print 'NETWORK: %d %d' % (num_pops, pop_size)
                res = scaling.build_network(num_pops, pop_size, self.marocco)
                results.append(res)

                if ester.exit_code():
                    # stop if ester server died. Ester will throw, but the
                    # unittest framework catches the exception and keep on
                    # going.
                    sys.exit(42)

    ScalingTest = type(
            'Test_numpops%d_popsize%d' % (num_pops, pop_size),
            (ScalingBase, ), {})

    testloader = unittest.TestLoader()
    testnames = testloader.getTestCaseNames(ScalingTest)
    suite = unittest.TestSuite()
    for name in testnames:
        suite.addTest(ScalingTest(name))
    return suite


def main():
    results = []

    suite = unittest.TestSuite()
    for num_pops in [ 1, 10, 25, 50, 100, 200, 300, 400 ]:
        for pop_size in [ 1, 16, 32, 64, 96 ]:
            suite.addTest(generate_network_scaling(num_pops, pop_size, results))

    unittest.TextTestRunner(verbosity=2).run(suite)

    # store reults in tempfile
    fname = tempfile.mktemp()
    np.savetxt(fname, results)
    print fname

    scaling.plot(np.array(results))
    import pylab
    plotfile = tempfile.mktemp()
    pylab.savefig('%s.png' % plotfile)


main()
