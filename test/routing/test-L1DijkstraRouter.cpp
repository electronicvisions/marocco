#include "test/common.h"

#include <set>

#include "hal/Coordinate/HICANN.h"
#include "hal/Coordinate/iter_all.h"
#include "marocco/routing/L1DijkstraRouter.h"
#include "marocco/routing/L1Routing.h"

using namespace HMF::Coordinate;

namespace marocco {
namespace routing {

class AL1DijkstraRouter : public ::testing::Test
{
protected:
	static void SetUpTestCase()
	{
		for (auto hicann : iter_all<HICANNOnWafer>()) {
			routing_graph.add(hicann);
		}
	}

	static L1RoutingGraph routing_graph;
}; // AL1DijkstraRouter

L1RoutingGraph AL1DijkstraRouter::routing_graph;

TEST_F(AL1DijkstraRouter, findsAllCandidatesForHorizontalTargets)
{
	HICANNOnWafer hicann1(X(5), Y(5));
	HICANNOnWafer hicann2(X(6), Y(5));
	L1EdgeWeights weights(routing_graph.graph());
	L1DijkstraRouter dijkstra(
	    weights, routing_graph[hicann1][SendingRepeaterOnHICANN(3).toHLineOnHICANN()]);
	Target target(hicann2, horizontal);
	dijkstra.add_target(target);
	dijkstra.run();
	EXPECT_EQ(1, dijkstra.vertices_for(target).size());
	std::set<L1RoutingGraph::vertex_descriptor> vertices,
		reference{routing_graph[hicann2][HLineOnHICANN(32)]};
	for (auto const& vertex : dijkstra.vertices_for(target)) {
		vertices.insert(vertex);
	}
	ASSERT_TRUE(std::equal(vertices.begin(), vertices.end(), reference.begin()));
}

TEST_F(AL1DijkstraRouter, findsPathToHorizontalTarget)
{
	HICANNOnWafer hicann1(X(5), Y(5));
	HICANNOnWafer hicann2(X(6), Y(5));
	L1EdgeWeights weights(routing_graph.graph());
	L1DijkstraRouter dijkstra(
	    weights, routing_graph[hicann1][SendingRepeaterOnHICANN(3).toHLineOnHICANN()]);
	Target target(hicann2, horizontal);
	dijkstra.add_target(target);
	dijkstra.run();
	auto const vertex = routing_graph[hicann2][HLineOnHICANN(32)];
	auto path = dijkstra.path_to(vertex);
	ASSERT_FALSE(path.empty());
	EXPECT_EQ(
		L1BusOnWafer(hicann1, SendingRepeaterOnHICANN(3).toHLineOnHICANN()),
	    routing_graph[path.front()]);
	EXPECT_EQ(L1BusOnWafer(hicann2, HLineOnHICANN(32)), routing_graph[path.back()]);
	L1Route route;
	ASSERT_NO_THROW(route = toL1Route(routing_graph.graph(), path));
	L1Route reference{HICANNOnWafer(X(5), Y(5)), HLineOnHICANN(30), HICANNOnWafer(X(6), Y(5)),
	                  HLineOnHICANN(32)};
	EXPECT_EQ(reference, route);
}

TEST_F(AL1DijkstraRouter, findsAllCandidatesForVerticalTargets)
{
	HICANNOnWafer hicann1(X(5), Y(5));
	HICANNOnWafer hicann2(X(6), Y(5));
	L1EdgeWeights weights(routing_graph.graph());
	L1DijkstraRouter dijkstra(
	    weights, routing_graph[hicann1][SendingRepeaterOnHICANN(3).toHLineOnHICANN()]);
	Target target(hicann2, vertical);
	dijkstra.add_target(target);
	dijkstra.run();
	EXPECT_EQ(1, dijkstra.vertices_for(target).size());
	std::set<L1RoutingGraph::vertex_descriptor> vertices,
		reference{routing_graph[hicann2][VLineOnHICANN(15)]};
	for (auto const& vertex : dijkstra.vertices_for(target)) {
		vertices.insert(vertex);
	}
	ASSERT_TRUE(std::equal(vertices.begin(), vertices.end(), reference.begin()));
}

TEST_F(AL1DijkstraRouter, findsPathToVerticalTarget)
{
	HICANNOnWafer hicann1(X(5), Y(5));
	HICANNOnWafer hicann2(X(6), Y(5));
	L1EdgeWeights weights(routing_graph.graph());
	L1DijkstraRouter dijkstra(
	    weights, routing_graph[hicann1][SendingRepeaterOnHICANN(3).toHLineOnHICANN()]);
	Target target(hicann2, vertical);
	dijkstra.add_target(target);
	dijkstra.run();
	auto const vertex = routing_graph[hicann2][VLineOnHICANN(79)];
	auto path = dijkstra.path_to(vertex);
	ASSERT_FALSE(path.empty());
	EXPECT_EQ(
		L1BusOnWafer(hicann1, SendingRepeaterOnHICANN(3).toHLineOnHICANN()),
		routing_graph[path.front()]);
	EXPECT_EQ(L1BusOnWafer(hicann2, VLineOnHICANN(79)), routing_graph[path.back()]);
	L1Route route;
	ASSERT_NO_THROW(route = toL1Route(routing_graph.graph(), path));
	L1Route reference{HICANNOnWafer(X(5), Y(5)), HLineOnHICANN(30), HICANNOnWafer(X(6), Y(5)),
	                  HLineOnHICANN(32), VLineOnHICANN(79)};
	EXPECT_EQ(reference, route);
}

TEST_F(AL1DijkstraRouter, doesNotLoopWithinASingleHICANN)
{
	// In the following example we do not want the first result:
	// $ ./bin/tools/find_route.py --source-hicann 368 --target-hicann 367 --source-hline 8
	// (L1Route HICANNOnWafer(X(20), Y(14))
	//   HLineOnHICANN(8)
	//   HICANNOnWafer(X(19), Y(14))
	//   HLineOnHICANN(6)
	//   VLineOnHICANN(28)
	//   HLineOnHICANN(7))
	// (L1Route HICANNOnWafer(X(20), Y(14))
	//   HLineOnHICANN(8)
	//   HICANNOnWafer(X(19), Y(14))
	//   HLineOnHICANN(6))

	HICANNOnWafer source_hicann(Enum(368));
	HICANNOnWafer target_hicann(Enum(367));
	L1EdgeWeights weights(routing_graph.graph());
	L1DijkstraRouter dijkstra(
		weights, routing_graph[source_hicann][HLineOnHICANN(8)]);
	Target target(target_hicann, horizontal);
	dijkstra.add_target(target);
	dijkstra.run();

	std::set<L1RoutingGraph::vertex_descriptor> vertices;
	for (auto const& vertex : dijkstra.vertices_for(target)) {
		vertices.insert(vertex);
	}

	EXPECT_NE(vertices.end(), vertices.find(routing_graph[target_hicann][HLineOnHICANN(6)]));
	EXPECT_EQ(vertices.end(), vertices.find(routing_graph[target_hicann][HLineOnHICANN(7)]));
}

TEST_F(AL1DijkstraRouter, doesNotUseMoreThanOneSwitchPerBusTwoHICANNs)
{
	// ❯ bin/tools/find_route.py --source-hicann 303 --target-hicann 275 --source-vline 8 --target-vertical
	// (L1Route HICANNOnWafer(Enum(303))
	//          VLineOnHICANN(8)
	//          HICANNOnWafer(Enum(275))
	//          VLineOnHICANN(10)         |
	//          HLineOnHICANN(42)         | <-- two switches for this hline
	//          VLineOnHICANN(74))        |
	// (L1Route HICANNOnWafer(Enum(303))
	//          VLineOnHICANN(8)          |
	//          HLineOnHICANN(46)         | <-- two switches for this hline
	//          VLineOnHICANN(104)        |
	//          HICANNOnWafer(Enum(275))
	//          VLineOnHICANN(106))
	// (L1Route HICANNOnWafer(Enum(303))    <-- only this route is okay
	//          VLineOnHICANN(8)
	//          HICANNOnWafer(Enum(275))
	//          VLineOnHICANN(10))

	HICANNOnWafer source_hicann(Enum(303));
	HICANNOnWafer target_hicann(Enum(275));
	L1EdgeWeights weights(routing_graph.graph());
	L1DijkstraRouter dijkstra(
		weights, routing_graph[source_hicann][VLineOnHICANN(8)]);
	Target target(target_hicann, vertical);
	dijkstra.add_target(target);
	dijkstra.run();

	std::set<L1RoutingGraph::vertex_descriptor> vertices;
	for (auto const& vertex : dijkstra.vertices_for(target)) {
		vertices.insert(vertex);
	}

	ASSERT_EQ(1, vertices.size());
	auto path = dijkstra.path_to(*(vertices.begin()));
	auto route = toL1Route(routing_graph.graph(), path);
	L1Route reference{HICANNOnWafer(Enum(303)), VLineOnHICANN(8), HICANNOnWafer(Enum(275)),
	                  VLineOnHICANN(10)};
	EXPECT_EQ(reference, route);
}

TEST_F(AL1DijkstraRouter, doesNotUseMoreThanOneSwitchPerBusSameHICANN)
{
	// ❯ bin/tools/find_route.py --source-hicann 303 --target-hicann 303 --source-vline 8 --target-vertical
	// (L1Route HICANNOnWafer(Enum(303))
	//          VLineOnHICANN(8)          |
	//          HLineOnHICANN(46)         | <-- two switches for this hline
	//          VLineOnHICANN(104))       |
	// (L1Route HICANNOnWafer(Enum(303))    <-- this route is okay
	//          VLineOnHICANN(8))

	HICANNOnWafer source_hicann(Enum(303));
	HICANNOnWafer target_hicann(Enum(303));
	L1EdgeWeights weights(routing_graph.graph());
	L1DijkstraRouter dijkstra(
		weights, routing_graph[source_hicann][VLineOnHICANN(8)]);
	Target target(target_hicann, vertical);
	dijkstra.add_target(target);
	dijkstra.run();

	std::set<L1RoutingGraph::vertex_descriptor> vertices;
	for (auto const& vertex : dijkstra.vertices_for(target)) {
		vertices.insert(vertex);
	}

	ASSERT_EQ(1, vertices.size());
	auto path = dijkstra.path_to(*(vertices.begin()));
	auto route = toL1Route(routing_graph.graph(), path);
	L1Route reference{HICANNOnWafer(Enum(303)), VLineOnHICANN(8)};
	EXPECT_EQ(reference, route);
}

} // routing
} // marocco
