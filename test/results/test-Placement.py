import unittest

from pyhalco_hicann_v2 import *
from pyhalco_common import *
from pyhalbe.HICANN import L1Address
from pymarocco_coordinates import BioNeuron, LogicalNeuron, L1AddressOnWafer
from pymarocco_results import Placement


class PlacementTest(unittest.TestCase):
    def test_item(self):
        bio = BioNeuron(42, 5)
        logical = LogicalNeuron.external(13)
        item = Placement.item_type(bio, logical)
        self.assertIsNone(item.neuron_block())
        self.assertIsNone(item.dnc_merger())
        self.assertEqual(bio, item.bio_neuron())
        self.assertEqual(bio.population(), item.population())
        self.assertEqual(bio.neuron_index(), item.neuron_index())
        self.assertEqual(logical, item.logical_neuron())
        self.assertIsNone(item.address())

    def test_add(self):
        pl = Placement()
        pl.add(BioNeuron(42, 0), LogicalNeuron.external(13))
        pl.add(
            BioNeuron(123, 0),
            (LogicalNeuron.on(NeuronBlockOnWafer())
             .add(NeuronOnNeuronBlock(X(2), Y(0)), 4)
             .add(NeuronOnNeuronBlock(X(3), Y(1)), 2)
             .done()))
        return pl

    def test_iterable(self):
        pl = self.test_add()
        for item in pl:
            self.assertIsNone(item.address())
        items = list(pl)
        self.assertEqual(2, len(items))

    def test_find_by_population(self):
        pl = self.test_add()
        items = list(pl.find(123))
        self.assertEqual(1, len(items))
        self.assertEqual(123, items[0].population())

    def test_find_by_bio_neuron(self):
        pl = self.test_add()
        items = list(pl.find(BioNeuron(42, 0)))
        self.assertEqual(1, len(items))
        self.assertEqual(42, items[0].population())

    def test_find_by_logical_neuron(self):
        pl = self.test_add()
        items = list(pl.find(LogicalNeuron.external(13)))
        self.assertEqual(1, len(items))
        self.assertEqual(42, items[0].population())

    def test_find_by_neuron_block(self):
        pl = self.test_add()
        items = list(pl.find(NeuronBlockOnWafer()))
        self.assertEqual(1, len(items))
        self.assertEqual(123, items[0].population())

    def test_set_address(self):
        pl = self.test_add()
        nrn = LogicalNeuron.external(13)

        items = list(pl.find(nrn))
        self.assertEqual(1, len(items))
        self.assertIsNone(items[0].address())

        address = L1AddressOnWafer(
            DNCMergerOnWafer(DNCMergerOnHICANN(3)), L1Address(42))
        pl.set_address(LogicalNeuron.external(13), address)

        items = list(pl.find(nrn))
        self.assertEqual(1, len(items))
        self.assertIsNotNone(items[0].address())
        self.assertEqual(address, items[0].address())

        return pl

    def test_find_by_dnc_merger(self):
        pl = self.test_set_address()
        items = list(pl.find(DNCMergerOnWafer(DNCMergerOnHICANN(3))))
        self.assertEqual(1, len(items))
        self.assertEqual(42, items[0].population())



if __name__ == "__main__":
    unittest.main()
