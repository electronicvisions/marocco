import unittest

from pyhalbe.Coordinate import *
from pymarocco_coordinates import L1Route


class L1RouteTest(unittest.TestCase):
    def test_append(self):
        route = L1Route()

        self.assertRaises(RuntimeError, route.append,
                          HICANNOnWafer(X(5), Y(5)))
        self.assertRaises(RuntimeError, route.append,
                          HICANNOnWafer(X(5), Y(5)), HICANNOnWafer(X(5), Y(6)))
        route.append(HICANNOnWafer(X(5), Y(5)), Merger0OnHICANN(2))
        self.assertRaises(RuntimeError, route.append,
                          HLineOnHICANN(42))
        self.assertRaises(RuntimeError, route.append,
                          HICANNOnWafer(X(6), Y(5)), HLineOnHICANN(48))
        route.append(DNCMergerOnHICANN(2))
        route.append(HLineOnHICANN(46))
        self.assertRaises(RuntimeError, route.append,
                          HLineOnHICANN(42))
        self.assertRaises(RuntimeError, route.append,
                          VLineOnHICANN(39))
        self.assertRaises(RuntimeError, route.append,
                          HICANNOnWafer(X(6), Y(5)), HLineOnHICANN(42))
        route.append(HICANNOnWafer(X(6), Y(5)), HLineOnHICANN(48))
        route.append(VLineOnHICANN(39))
        route.append(SynapseDriverOnHICANN(left, Y(99)))
        route.append(SynapseOnHICANN(SynapseColumnOnHICANN(5),
                                     SynapseRowOnHICANN(0)))

        return route

    def test_empty(self):
        route = L1Route()
        self.assertTrue(route.empty())
        route = self.test_append()
        self.assertFalse(route.empty())

    def test_source_and_target_hicann(self):
        route = self.test_append()
        self.assertEqual(HICANNOnWafer(X(5), Y(5)), route.source_hicann())
        self.assertEqual(HICANNOnWafer(X(6), Y(5)), route.target_hicann())

    def test_comparison(self):
        route = self.test_append()
        empty_route = L1Route()
        self.assertEqual(route, route)
        self.assertNotEqual(route, empty_route)
        self.assertTrue(route != empty_route)
        self.assertFalse(route == empty_route)

    def test_access_to_last_element(self):
        route = L1Route()

        route.append(HICANNOnWafer(X(5), Y(5)), Merger0OnHICANN(2))
        self.assertEqual(Merger0OnHICANN(2), route.back())
        route.append(DNCMergerOnHICANN(2))
        self.assertEqual(DNCMergerOnHICANN(2), route.back())

    def test_iteration(self):
        route = self.test_append()

        segments = [1 for segment in route]
        self.assertEqual(9, sum(segments))

    def test_access_to_segments(self):
        route = self.test_append()

        segments = list(route)
        self.assertEqual(9, len(segments))
        self.assertEqual(HICANNOnWafer(X(5), Y(5)), segments[0])
        self.assertEqual(SynapseDriverOnHICANN(left, Y(99)), segments[7])

    def test_access_via_getitem_is_readonly(self):
        route = self.test_append()

        self.assertEqual(HICANNOnWafer(X(5), Y(5)), route[0])
        self.assertEqual(Merger0OnHICANN(2), route[1])
        with self.assertRaises(TypeError):
            route[2] = HLineOnHICANN(Enum(5))

    def test_supports_pickling(self):
        import pickle
        route = self.test_append()
        d = pickle.dumps(route)
        self.assertEqual(route, pickle.loads(d))


if __name__ == "__main__":
    unittest.main()
