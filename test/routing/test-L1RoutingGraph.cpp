#include "test/common.h"

#include <set>

#include "hal/Coordinate/iter_all.h"
#include "marocco/routing/L1RoutingGraph.h"
#include "marocco/util/iterable.h"

using namespace HMF::Coordinate;

namespace marocco {
namespace routing {

TEST(L1RoutingGraph, providesAccessToBoostGraph)
{
	L1RoutingGraph rgraph;
	L1RoutingGraph::graph_type& bgraph = rgraph.graph();
	L1RoutingGraph::vertex_descriptor vertex = add_vertex(bgraph);
	remove_vertex(vertex, bgraph);
}

TEST(L1RoutingGraph, doesNotContainAllHICANNsByDefault)
{
	L1RoutingGraph rgraph;
	ASSERT_THROW(rgraph[HICANNOnWafer()], ResourceNotPresentError);
}

TEST(L1RoutingGraph, createsVerticesOnDemand)
{
	L1RoutingGraph rgraph;
	HICANNOnWafer coord;
	rgraph.add(coord);
	ASSERT_NO_THROW(rgraph[coord]);
}

template <typename LineT>
void check_for_edge_impl(
    L1RoutingGraph& rgraph,
    HICANNOnWafer const& hicann,
    LineT const& line,
    HICANNOnWafer (HICANNOnWafer::*conv)() const,
    LineT (LineT::*line_conv)() const,
    bool add = false)
{
	HICANNOnWafer adjacent;
	try {
		adjacent = (hicann.*conv)();
	} catch (std::overflow_error const&) {
		return;
	} catch (std::domain_error const&) {
		return;
	}

	if (add) {
		rgraph.add(hicann);
		rgraph.add(adjacent);
	}

	auto const& current = rgraph[hicann];
	auto const& other = rgraph[adjacent];
	auto adj_vertex = other[(line.*line_conv)()];
	auto its = adjacent_vertices(current[line], rgraph.graph());
	bool edge_present = std::any_of(
		its.first, its.second,
		[&adj_vertex](L1RoutingGraph::vertex_descriptor vertex) { return vertex == adj_vertex; });
	ASSERT_TRUE(edge_present);
}

TEST(L1RoutingGraph, setsUpEdgesBetweenTwoAdjacentHICANNs)
{
	{
		L1RoutingGraph rgraph;
		check_for_edge_impl(
		    rgraph, HICANNOnWafer(X(5), Y(5)), VLineOnHICANN(11), &HICANNOnWafer::north,
		    &VLineOnHICANN::north, true);
	}
	{
		L1RoutingGraph rgraph;
		check_for_edge_impl(
		    rgraph, HICANNOnWafer(X(5), Y(5)), HLineOnHICANN(11), &HICANNOnWafer::east,
		    &HLineOnHICANN::east, true);
	}
	{
		L1RoutingGraph rgraph;
		check_for_edge_impl(
		    rgraph, HICANNOnWafer(X(5), Y(5)), VLineOnHICANN(11), &HICANNOnWafer::south,
		    &VLineOnHICANN::south, true);
	}
	{
		L1RoutingGraph rgraph;
		check_for_edge_impl(
		    rgraph, HICANNOnWafer(X(5), Y(5)), HLineOnHICANN(11), &HICANNOnWafer::west,
		    &HLineOnHICANN::west, true);
	}
}

TEST(L1RoutingGraph, hasCorrectConnectivityWhenFullyPopulated)
{
	L1RoutingGraph rgraph;
	for (auto hicann : iter_all<HICANNOnWafer>()) {
		rgraph.add(hicann);
	}

	EXPECT_EQ(
	    (VLineOnHICANN::size + HLineOnHICANN::size) * HICANNOnWafer::enum_type::size,
	    num_vertices(rgraph.graph()));

	auto const& graph = rgraph.graph();
	for (auto hicann : iter_all<HICANNOnWafer>()) {
		auto const& hgraph = rgraph[hicann];

		for (auto hline : iter_all<HLineOnHICANN>()) {
			L1BusOnWafer bus = graph[hgraph[hline]];
			ASSERT_TRUE(bus.is_horizontal());
			ASSERT_FALSE(bus.is_vertical());
			ASSERT_EQ(hline, bus.toHLineOnHICANN());
			check_for_edge_impl(rgraph, hicann, hline, &HICANNOnWafer::east, &HLineOnHICANN::east);
			check_for_edge_impl(rgraph, hicann, hline, &HICANNOnWafer::west, &HLineOnHICANN::west);
		}

		for (auto vline : iter_all<VLineOnHICANN>()) {
			L1BusOnWafer bus = graph[hgraph[vline]];
			ASSERT_FALSE(bus.is_horizontal());
			ASSERT_TRUE(bus.is_vertical());
			ASSERT_EQ(vline, bus.toVLineOnHICANN());
			check_for_edge_impl(
			    rgraph, hicann, vline, &HICANNOnWafer::north, &VLineOnHICANN::north);
			check_for_edge_impl(
			    rgraph, hicann, vline, &HICANNOnWafer::south, &VLineOnHICANN::south);
		}
	}
}

TEST(L1RoutingGraph, satisfiesSomeArbitraryConnectivityCheckOnHICANN)
{
	L1RoutingGraph rgraph;
	rgraph.add(HICANNOnWafer());
	L1RoutingGraph::HICANN hicann = rgraph[HICANNOnWafer()];

	std::vector<L1RoutingGraph::vertex_descriptor> vertices;

	VLineOnHICANN vline(39);
	HLineOnHICANN hline(48);

	auto its = adjacent_vertices(hicann[vline], rgraph.graph());
	std::copy(its.first, its.second, std::back_inserter(vertices));
	EXPECT_EQ(2, vertices.size());
	bool edge_present = std::any_of(
		vertices.begin(), vertices.end(),
		[&hicann, &hline](L1RoutingGraph::vertex_descriptor vertex) {
			return vertex == hicann[hline];
		});
	EXPECT_TRUE(edge_present);

	vertices.clear();

	its = adjacent_vertices(hicann[hline], rgraph.graph());
	std::copy(its.first, its.second, std::back_inserter(vertices));
	EXPECT_EQ(8, vertices.size());
}

TEST(L1RoutingGraph, usesTheRightL1BusOnWaferVariants)
{
	L1RoutingGraph rgraph;
	rgraph.add(HICANNOnWafer());

	std::set<HLineOnHICANN> hlines;
	std::set<VLineOnHICANN> vlines;

	auto const& graph = rgraph.graph();
	for (auto const vertex : make_iterable(vertices(graph))) {
		auto const& bus = graph[vertex];
		if (bus.is_horizontal()) {
			EXPECT_TRUE(hlines.insert(bus.toHLineOnHICANN()).second);
		} else {
			EXPECT_TRUE(vlines.insert(bus.toVLineOnHICANN()).second);
		}
	}

	EXPECT_EQ(HLineOnHICANN::size, hlines.size());
	EXPECT_EQ(VLineOnHICANN::size, vlines.size());
}

// TODO: respect hardware defects by removing edges (repeaters) or vertices (buses).
/*
TEST(L1RoutingGraph, allowsDisablingOfDefectResources)
{
	L1RoutingGraph rgraph;
	HICANNOnWafer hicann;
	rgraph.add(hicann);
	// Option (a)
	rgraph.remove(hicann, HLineOnHICANN(39));
	// Option (b)
	rgraph[hicann].remove(HLineOnHICANN(39));
}
*/

} // namespace routing
} // namespace marocco
