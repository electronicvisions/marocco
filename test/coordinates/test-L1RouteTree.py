import itertools
import unittest

from pyhalco_hicann_v2 import *
from pyhalco_common import *
from pymarocco_coordinates import L1Route, L1RouteTree


class L1RouteTreeTest(unittest.TestCase):
    def test_permutations_have_same_result(self):
        routes = []

        route = L1Route()
        route.append(HICANNOnWafer(X(6), Y(5)), HLineOnHICANN(48))
        route.append(VLineOnHICANN(39))
        route.append(HLineOnHICANN(49))
        route.append(VLineOnHICANN(7))
        routes.append(route)

        route = L1Route()
        route.append(HICANNOnWafer(X(6), Y(5)), HLineOnHICANN(48))
        route.append(VLineOnHICANN(39))
        routes.append(route)

        route = L1Route()
        route.append(HICANNOnWafer(X(6), Y(5)), HLineOnHICANN(48))
        routes.append(route)

        route = L1Route()
        route.append(HICANNOnWafer(X(6), Y(5)), HLineOnHICANN(48))
        route.append(VLineOnHICANN(7))
        routes.append(route)

        route = L1Route()
        route.append(HICANNOnWafer(X(6), Y(5)), HLineOnHICANN(48))
        route.append(VLineOnHICANN(71))
        routes.append(route)

        route = L1Route()
        route.append(HICANNOnWafer(X(6), Y(5)), HLineOnHICANN(48))
        route.append(HICANNOnWafer(X(5), Y(5)), HLineOnHICANN(46))
        routes.append(route)

        route = L1Route()
        route.append(HICANNOnWafer(X(6), Y(5)), HLineOnHICANN(48))
        route.append(VLineOnHICANN(39))
        route.append(SynapseDriverOnHICANN(left, Y(99)))
        route.append(SynapseOnHICANN(SynapseColumnOnHICANN(5),
                                     SynapseRowOnHICANN(0)))
        routes.append(route)

        for N in range(2, len(routes)):
            for subset in itertools.combinations(routes, N):
                trees = []
                for perm in itertools.permutations(subset, N):
                    tree = L1RouteTree()
                    for route in perm:
                        tree.add(route)
                    if trees:
                        self.assertEqual(trees[0], tree)

        return routes

    def test_routes_fulfilled_en_passant_have_one_empty_tail(self):
        route = L1Route()
        route.append(HICANNOnWafer(X(5), Y(5)), Merger0OnHICANN(2))
        route.append(DNCMergerOnHICANN(2))
        route.append(HLineOnHICANN(46))
        route.append(HICANNOnWafer(X(6), Y(5)), HLineOnHICANN(48))
        route.append(VLineOnHICANN(39))
        route.append(SynapseDriverOnHICANN(left, Y(99)))
        route.append(SynapseOnHICANN(SynapseColumnOnHICANN(5),
                                     SynapseRowOnHICANN(0)))

        hicann = None
        route_ = L1Route()
        for segment in route:
            if isinstance(segment, HICANNOnWafer):
                hicann = segment
                continue
            if hicann is not None:
                route_.append(hicann, segment)
                hicann = None
            else:
                route_.append(segment)

            if route_ == route:
                continue

            tree = L1RouteTree(route)
            tree.add(route_)
            self.assertEqual(route_, tree.head())
            self.assertTrue(tree.has_tails())
            tails = tree.tails()
            self.assertEqual(2, len(tails))
            self.assertEqual(route.back(), tails[0].head().back())
            self.assertTrue(tails[1].empty())

    def test_tree_can_not_be_modified_via_head(self):
        route = L1Route()
        route.append(HICANNOnWafer(X(5), Y(5)), Merger0OnHICANN(2))
        route.append(DNCMergerOnHICANN(2))

        tree = L1RouteTree(route)
        copy = L1RouteTree(tree)
        # tree.head() should return a copy
        tree.head().append(HLineOnHICANN(46))
        self.assertEqual(copy, tree)

    def test_tree_can_not_be_modified_via_tails(self):
        route = L1Route()
        route.append(HICANNOnWafer(X(5), Y(5)), Merger0OnHICANN(2))
        route.append(DNCMergerOnHICANN(2))
        route.append(HLineOnHICANN(46))

        route_ = L1Route()
        route_.append(HICANNOnWafer(X(5), Y(5)), Merger0OnHICANN(2))
        route_.append(DNCMergerOnHICANN(2))

        tree = L1RouteTree(route)
        tree.add(route_)
        copy = L1RouteTree(tree)
        # tree.tails() should return copies
        tails = tree.tails()
        self.assertFalse(tails[0].empty())
        route = L1Route()
        route.append(HICANNOnWafer(X(5), Y(5)), HLineOnHICANN(46))
        route.append(VLineOnHICANN(8))
        tails[0].add(route)
        self.assertEqual(copy, tree)

    def test_supports_pickling(self):
        import pickle
        for route in self.test_permutations_have_same_result():
            d = pickle.dumps(route)
            self.assertEqual(route, pickle.loads(d))


if __name__ == "__main__":
    unittest.main()
