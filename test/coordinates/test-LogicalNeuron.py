import unittest

from pyhalbe.Coordinate import *
from pymarocco_coordinates import NodeOnFPGA, LogicalNeuron


class LogicalNeuronTest(unittest.TestCase):
    def test_external_node(self):
        src = NodeOnFPGA(15)
        nrn = LogicalNeuron(Wafer(), src)

        self.assertTrue(nrn.is_external())
        self.assertEqual(src, nrn.external_node())
        self.assertEqual(0, len(nrn.denmems()))
        self.assertSequenceEqual([], nrn.denmems())

    def test_denmems(self):
        h = HICANNGlobal()
        a = NeuronGlobal(NeuronOnHICANN(Enum(3)), h)
        b = NeuronGlobal(NeuronOnHICANN(Enum(42)), h)
        nrn = LogicalNeuron(Wafer(5), [a, b])

        self.assertEqual(Wafer(5), nrn.wafer())
        self.assertFalse(nrn.is_external())
        with self.assertRaises(RuntimeError):
            _ = nrn.external_node()

        self.assertSequenceEqual([a, b], nrn.denmems())

    def test_denmems_empty(self):
        with self.assertRaises(RuntimeError):
            nrn = LogicalNeuron(Wafer(), [])


if __name__ == "__main__":
    unittest.main()
