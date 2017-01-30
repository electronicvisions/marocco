import unittest

from pyhalbe.Coordinate import *
from pymarocco_coordinates import LogicalNeuron


class LogicalNeuronTest(unittest.TestCase):
    def test_external_node(self):
        nrn = LogicalNeuron.external(15)

        self.assertTrue(nrn.is_external())
        with self.assertRaises(RuntimeError):
            nrn.size()
        self.assertSequenceEqual([], list(nrn))
        with self.assertRaises(RuntimeError):
            nrn.denmem(0)

        self.assertEqual(LogicalNeuron.external(15), nrn)
        self.assertNotEqual(LogicalNeuron.external(12), nrn)
        self.assertEqual(15, nrn.external_identifier())
        self.assertEqual(0, nrn.external_index())

        nrn = LogicalNeuron.external(42, 2)
        self.assertEqual(42, nrn.external_identifier())
        self.assertEqual(2, nrn.external_index())

        self.assertTrue(str(nrn).startswith('LogicalNeuron::external('))

    def test_builder(self):
        nrn = (LogicalNeuron.on(NeuronBlockOnWafer())
               .add(NeuronOnNeuronBlock(), 5)
               .done())
        self.assertFalse(nrn.is_external())
        self.assertTrue(str(nrn).startswith('LogicalNeuron::on('))
        with self.assertRaises(RuntimeError):
            nrn.external_identifier()
        with self.assertRaises(RuntimeError):
            nrn.external_index()

        with self.assertRaises(ValueError):
            no_chunks_specified = (
                LogicalNeuron.on(NeuronBlockOnWafer()).done())

    def test_rectangular(self):
        nrn = LogicalNeuron.rectangular(
            NeuronOnWafer(NeuronOnHICANN(X(3), Y(0))), 4)
        reference_denmems = [
            NeuronOnWafer(NeuronOnHICANN(X(3), Y(0))),
            NeuronOnWafer(NeuronOnHICANN(X(4), Y(0))),
            NeuronOnWafer(NeuronOnHICANN(X(3), Y(1))),
            NeuronOnWafer(NeuronOnHICANN(X(4), Y(1))),
        ]
        self.assertFalse(nrn.is_external())
        self.assertEqual(4, nrn.size())
        self.assertSequenceEqual(reference_denmems, list(nrn))
        self.assertTrue(str(nrn).startswith('LogicalNeuron::on('))
        with self.assertRaises(RuntimeError):
            nrn.external_identifier()
        with self.assertRaises(RuntimeError):
            nrn.external_index()

        with self.assertRaises(ValueError):
            wrong_size = LogicalNeuron.rectangular(
                NeuronOnWafer(NeuronOnHICANN(X(3), Y(0))), 5)

        with self.assertRaises(ValueError):
            wrong_row = LogicalNeuron.rectangular(
                NeuronOnWafer(NeuronOnHICANN(X(3), Y(1))), 4)

    def test_denmems(self):
        # |  XXXX
        # |   XX
        nrn = (LogicalNeuron.on(NeuronBlockOnWafer())
               .add(NeuronOnNeuronBlock(X(2), Y(0)), 4)
               .add(NeuronOnNeuronBlock(X(3), Y(1)), 2)
               .done())
        reference_denmems = [
            NeuronOnWafer(NeuronOnHICANN(X(2), Y(0))),
            NeuronOnWafer(NeuronOnHICANN(X(3), Y(0))),
            NeuronOnWafer(NeuronOnHICANN(X(4), Y(0))),
            NeuronOnWafer(NeuronOnHICANN(X(5), Y(0))),
            NeuronOnWafer(NeuronOnHICANN(X(3), Y(1))),
            NeuronOnWafer(NeuronOnHICANN(X(4), Y(1))),
        ]

        self.assertFalse(nrn.is_external())
        self.assertEqual(6, nrn.size())
        self.assertEqual(reference_denmems[0], nrn.denmem(0))
        self.assertEqual(reference_denmems[1], nrn.denmem(1))
        self.assertEqual(reference_denmems[2], nrn.denmem(2))
        self.assertEqual(reference_denmems[3], nrn.denmem(3))
        self.assertEqual(reference_denmems[4], nrn.denmem(4))
        self.assertEqual(reference_denmems[5], nrn.denmem(5))
        with self.assertRaises(IndexError):
            nrn.denmem(6)
        self.assertSequenceEqual(reference_denmems, list(nrn))
        self.assertNotEqual(LogicalNeuron.external(12), nrn)
        self.assertEqual(nrn, nrn)

    def test_no_denmems(self):
        with self.assertRaises(ValueError):
            LogicalNeuron.on(NeuronBlockOnWafer()).done()


if __name__ == "__main__":
    unittest.main()
