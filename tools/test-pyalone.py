import unittest

from pyhalbe.Coordinate import *
from pymarocco_coordinates import L1Route
import pyalone
import pysthal


class AloneTest(unittest.TestCase):
    def test_basic_route(self):
        alone = pyalone.Alone()
        hicann1 = HICANNOnWafer(X(5), Y(5))
        hicann2 = HICANNOnWafer(X(6), Y(5))
        alone.add(hicann1)
        alone.add(hicann2)
        source = pyalone.L1BusOnWafer(
            hicann1, SendingRepeaterOnHICANN(3).toHLineOnHICANN())
        target = pyalone.Target(hicann2, horizontal)
        routes = alone.find_routes(source, target)
        self.assertEqual(1, len(routes))
        for route in routes:
            self.assertFalse(route.empty())
            self.assertEqual(hicann1, route.source_hicann())
            self.assertEqual(hicann2, route.target_hicann())
        return routes

    def test_configure(self):
        route = L1Route()
        hicann = HICANNOnWafer(X(6), Y(5))
        route.append(hicann, HLineOnHICANN(48))
        route.append(VLineOnHICANN(39))
        wafer = pysthal.Wafer()
        pyalone.configure(wafer, route)
        self.assertEqual(1, len(wafer.getAllocatedHicannCoordinates()))
        reference = pysthal.HICANN()
        reference.crossbar_switches.set(
            VLineOnHICANN(39), HLineOnHICANN(48), True)
        self.assertEqual(reference, wafer[hicann])

    def test_dnc_merger_to_right(self):
        alone = pyalone.Alone()
        hicann_left = HICANNOnWafer(X(20), Y(14))
        hicann_right = HICANNOnWafer(X(21), Y(14))
        alone.add(hicann_left)
        alone.add(hicann_right)
        merger = DNCMergerOnHICANN(2)
        target = pyalone.Target(hicann_right, vertical)
        routes = alone.find_routes(hicann_left, merger, target)
        self.assertGreater(len(routes), 1)
        for route in routes:
            self.assertFalse(route.empty())
            self.assertEqual(hicann_left, route.source_hicann())
            self.assertEqual(hicann_right, route.target_hicann())
            segments = list(route)
            self.assertEqual(hicann_left, segments[0])
            self.assertEqual(merger, segments[1])
            self.assertEqual(HLineOnHICANN(46), segments[2])
            self.assertEqual(hicann_right, segments[3])

    def test_dnc_merger_to_left(self):
        alone = pyalone.Alone()
        hicann_left = HICANNOnWafer(X(20), Y(14))
        hicann_right = HICANNOnWafer(X(21), Y(14))
        alone.add(hicann_left)
        alone.add(hicann_right)
        merger = DNCMergerOnHICANN(2)
        target = pyalone.Target(hicann_left, vertical)
        routes = alone.find_routes(hicann_right, merger, target)
        self.assertGreater(len(routes), 1)
        for route in routes:
            self.assertFalse(route.empty())
            self.assertEqual(hicann_right, route.source_hicann())
            self.assertEqual(hicann_left, route.target_hicann())
            segments = list(route)
            self.assertEqual(hicann_right, segments[0])
            self.assertEqual(merger, segments[1])
            self.assertEqual(hicann_left, segments[2])


if __name__ == "__main__":
    unittest.main()
