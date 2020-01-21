#include "test/common.h"

#include "halco/hicann/v2/hicann.h"
#include "halco/common/iter_all.h"
#include "marocco/routing/L1GraphWalker.h"
#include "marocco/routing/L1Routing.h"

using namespace halco::hicann::v2;
using namespace halco::common;

namespace marocco {
namespace routing {

class AL1GraphWalker : public ::testing::Test
{
protected:
	static void SetUpTestCase()
	{
		for (auto hicann : iter_all<HICANNOnWafer>()) {
			routing_graph.add(hicann);
		}
	}

	L1GraphWalker create_walker()
	{
		return {routing_graph.graph()};
	}

	size_t min_x = HICANNOnWafer::x_type::min;
	size_t max_x = HICANNOnWafer::x_type::max;
	size_t min_y = HICANNOnWafer::y_type::min;
	size_t max_y = HICANNOnWafer::y_type::max;
	static L1RoutingGraph routing_graph;
}; // AL1GraphWalker

L1RoutingGraph AL1GraphWalker::routing_graph;

TEST_F(AL1GraphWalker, canStepAlongHorizontalAxis)
{
	HICANNOnWafer hicann(X(5), Y(5));
	auto walker = create_walker();
	auto graph = walker.graph();
	auto current = routing_graph[hicann][HLineOnHICANN(32)];
	ASSERT_EQ(hicann, graph[current].toHICANNOnWafer());
	ASSERT_TRUE(walker.step(current, east));
	EXPECT_EQ(hicann.east(), graph[current].toHICANNOnWafer());
	ASSERT_TRUE(walker.step(current, west));
	EXPECT_EQ(hicann, graph[current].toHICANNOnWafer());
	ASSERT_TRUE(walker.step(current, west));
	EXPECT_EQ(hicann.west(), graph[current].toHICANNOnWafer());
}

TEST_F(AL1GraphWalker, canStepAlongVerticalAxis)
{
	HICANNOnWafer hicann(X(5), Y(5));
	auto walker = create_walker();
	auto graph = walker.graph();
	auto current = routing_graph[hicann][VLineOnHICANN(32)];
	ASSERT_EQ(hicann, graph[current].toHICANNOnWafer());
	ASSERT_TRUE(walker.step(current, north));
	EXPECT_EQ(hicann.north(), graph[current].toHICANNOnWafer());
	ASSERT_TRUE(walker.step(current, south));
	EXPECT_EQ(hicann, graph[current].toHICANNOnWafer());
	ASSERT_TRUE(walker.step(current, south));
	EXPECT_EQ(hicann.south(), graph[current].toHICANNOnWafer());
}

TEST_F(AL1GraphWalker, canAvoidSomeBuses)
{
	HICANNOnWafer hicann(X(5), Y(5));
	HLineOnHICANN hline(32);
	auto walker = create_walker();
	auto graph = walker.graph();
	auto current = routing_graph[hicann][hline];
	auto avoid = routing_graph[hicann.east()][hline.east()];
	walker.avoid_using(avoid);
	ASSERT_EQ(hicann, graph[current].toHICANNOnWafer());
	ASSERT_FALSE(walker.step(current, east));
	EXPECT_EQ(hicann, graph[current].toHICANNOnWafer());
}

TEST_F(AL1GraphWalker, respectsWaferBoundaries)
{
	auto walker = create_walker();
	auto graph = walker.graph();
	auto current = routing_graph[HICANNOnWafer(X(8), Y(2))][VLineOnHICANN(32)];
	ASSERT_FALSE(walker.step(current, north));
	EXPECT_EQ(HICANNOnWafer(X(8), Y(2)), graph[current].toHICANNOnWafer());
	current = routing_graph[HICANNOnWafer(X(20), Y(0))][VLineOnHICANN(32)];
	ASSERT_FALSE(walker.step(current, north));
	EXPECT_EQ(HICANNOnWafer(X(20), Y(0)), graph[current].toHICANNOnWafer());
}

TEST_F(AL1GraphWalker, canWalkAlongHorizontalAxis)
{
	HICANNOnWafer hicann(X(8), Y(5));
	auto walker = create_walker();
	auto graph = walker.graph();

	L1GraphWalker::vertex_descriptor vertex;
	L1GraphWalker::path_type path;
	HICANNOnWafer back, front;
	bool reached_limit;

	vertex = routing_graph[hicann][HLineOnHICANN(32)];
	std::tie(path, reached_limit) = walker.walk(vertex, east, max_x);
	EXPECT_FALSE(reached_limit);
	EXPECT_EQ(31 - 8, path.size()); // 31 is the maximum X coordinate for Y(5).
	back = graph[path.back()].toHICANNOnWafer();
	EXPECT_EQ(HICANNOnWafer(X(31), Y(5)), back) << back;
	front = graph[path.front()].toHICANNOnWafer();
	EXPECT_EQ(HICANNOnWafer(X(9), Y(5)), front) << front;

	vertex = routing_graph[hicann][HLineOnHICANN(32)];
	std::tie(path, reached_limit) = walker.walk(vertex, west, min_x);
	EXPECT_FALSE(reached_limit);
	EXPECT_EQ(8 - 4, path.size());
	back = graph[path.back()].toHICANNOnWafer();
	EXPECT_EQ(HICANNOnWafer(X(4), Y(5)), back) << back;
	front = graph[path.front()].toHICANNOnWafer();
	EXPECT_EQ(HICANNOnWafer(X(7), Y(5)), front) << front;
}

TEST_F(AL1GraphWalker, canWalkAlongHorizontalAxisWithLimit)
{
	HICANNOnWafer hicann(X(8), Y(5));
	auto walker = create_walker();
	auto graph = walker.graph();

	L1GraphWalker::vertex_descriptor vertex;
	L1GraphWalker::path_type path;
	HICANNOnWafer back;
	bool reached_limit;

	vertex = routing_graph[hicann][HLineOnHICANN(32)];
	std::tie(path, reached_limit) = walker.walk(vertex, east, 21);
	EXPECT_TRUE(reached_limit);
	back = graph[path.back()].toHICANNOnWafer();
	EXPECT_EQ(HICANNOnWafer(X(21), Y(5)), back) << back;

	vertex = routing_graph[hicann][HLineOnHICANN(32)];
	std::tie(path, reached_limit) = walker.walk(vertex, west, 5);
	EXPECT_TRUE(reached_limit);
	back = graph[path.back()].toHICANNOnWafer();
	EXPECT_EQ(HICANNOnWafer(X(5), Y(5)), back) << back;
}

TEST_F(AL1GraphWalker, canWalkAlongVerticalAxis)
{
	HICANNOnWafer hicann(X(8), Y(5));
	auto walker = create_walker();
	auto graph = walker.graph();

	L1GraphWalker::vertex_descriptor vertex;
	L1GraphWalker::path_type path;
	HICANNOnWafer back, front;
	bool reached_limit;

	vertex = routing_graph[hicann][VLineOnHICANN(32)];
	std::tie(path, reached_limit) = walker.walk(vertex, north, min_y);
	EXPECT_FALSE(reached_limit);
	EXPECT_EQ(5 - 2, path.size()); // 2 is the minimum Y for X(8).
	back = graph[path.back()].toHICANNOnWafer();
	EXPECT_EQ(HICANNOnWafer(X(8), Y(2)), back) << back;
	front = graph[path.front()].toHICANNOnWafer();
	EXPECT_EQ(HICANNOnWafer(X(8), Y(4)), front) << front;

	vertex = routing_graph[hicann][VLineOnHICANN(32)];
	std::tie(path, reached_limit) = walker.walk(vertex, south, max_y);
	EXPECT_FALSE(reached_limit);
	EXPECT_EQ(13 - 5, path.size());
	back = graph[path.back()].toHICANNOnWafer();
	EXPECT_EQ(HICANNOnWafer(X(8), Y(13)), back) << back;
	front = graph[path.front()].toHICANNOnWafer();
	EXPECT_EQ(HICANNOnWafer(X(8), Y(6)), front) << front;
}

TEST_F(AL1GraphWalker, doesNotWalkBeyondLimitIfAlreadyExceeded)
{
	HICANNOnWafer hicann(X(8), Y(5));
	auto walker = create_walker();
	auto graph = walker.graph();

	L1GraphWalker::vertex_descriptor vertex;
	L1GraphWalker::path_type path;
	bool reached_limit;

	vertex = routing_graph[hicann][HLineOnHICANN(32)];
	std::tie(path, reached_limit) = walker.walk(vertex, east, 8);
	EXPECT_TRUE(reached_limit);
	EXPECT_TRUE(path.empty());

	vertex = routing_graph[hicann][HLineOnHICANN(32)];
	std::tie(path, reached_limit) = walker.walk(vertex, east, 7);
	EXPECT_TRUE(reached_limit);
	EXPECT_TRUE(path.empty());
}

TEST_F(AL1GraphWalker, returnsViableVLinesForChangeOfOrientation)
{
	HICANNOnWafer hicann(X(8), Y(5));
	auto walker = create_walker();
	auto graph = walker.graph();

	auto vertex = routing_graph[hicann][HLineOnHICANN(32)];
	auto candidates = walker.change_orientation(vertex);
	EXPECT_EQ(8, candidates.size());
	EXPECT_EQ(hicann, graph[vertex].toHICANNOnWafer());

	walker.avoid_using(routing_graph[hicann][VLineOnHICANN(47)]);
	candidates = walker.change_orientation(vertex);
	EXPECT_EQ(7, candidates.size());
	EXPECT_EQ(hicann, graph[vertex].toHICANNOnWafer());
}

TEST_F(AL1GraphWalker, returnsViableHLinesForChangeOfOrientation)
{
	HICANNOnWafer hicann(X(8), Y(5));
	auto walker = create_walker();
	auto graph = walker.graph();

	auto vertex = routing_graph[hicann][VLineOnHICANN(47)];
	auto candidates = walker.change_orientation(vertex);
	EXPECT_EQ(2, candidates.size());
	EXPECT_EQ(hicann, graph[vertex].toHICANNOnWafer());

	walker.avoid_using(routing_graph[hicann][HLineOnHICANN(32)]);
	candidates = walker.change_orientation(vertex);
	EXPECT_EQ(1, candidates.size());
	EXPECT_EQ(hicann, graph[vertex].toHICANNOnWafer());
}

TEST_F(AL1GraphWalker, canWalkWithDetourAlongHorizontalAxis)
{
	HICANNOnWafer hicann(X(8), Y(5));
	auto walker = create_walker();
	auto graph = walker.graph();

	L1GraphWalker::vertex_descriptor vertex;
	L1GraphWalker::path_type path, detour;
	bool reached_limit;

	vertex = routing_graph[hicann][HLineOnHICANN(32)];
	std::tie(path, reached_limit) = walker.walk(vertex, east, 28);
	EXPECT_TRUE(reached_limit);
	std::tie(detour, reached_limit) = walker.detour_and_walk(vertex, east, 28);
	EXPECT_TRUE(reached_limit);
	// In addition to the horizontal segments present in the original path, the detour has
	// to (1) switch to a vertical line on the same HICANN (2) walk to an adjacent HICANN
	// (3) switch back to a horizontal line.
	EXPECT_EQ(path.size() + 3, detour.size());
	EXPECT_EQ(HICANNOnWafer(X(28), Y(5)), graph[path.back()].toHICANNOnWafer());

	L1BusOnWafer front = graph[detour.front()];
	L1BusOnWafer back = graph[detour.back()];
	EXPECT_EQ(graph[path.back()].toHICANNOnWafer().x(), back.toHICANNOnWafer().x());
	EXPECT_EQ(vertical, front.toOrientation());
	EXPECT_EQ(hicann, front.toHICANNOnWafer());
}

TEST_F(AL1GraphWalker, DetourDoesNotWalkBeyondLimitIfAlreadyExceeded)
{
	HICANNOnWafer hicann(X(8), Y(5));
	auto walker = create_walker();
	auto graph = walker.graph();

	L1GraphWalker::vertex_descriptor vertex;
	L1GraphWalker::path_type path;
	bool reached_limit;

	vertex = routing_graph[hicann][HLineOnHICANN(32)];
	std::tie(path, reached_limit) = walker.detour_and_walk(vertex, east, 8);
	EXPECT_TRUE(reached_limit);
	EXPECT_TRUE(path.empty());

	vertex = routing_graph[hicann][HLineOnHICANN(32)];
	std::tie(path, reached_limit) = walker.detour_and_walk(vertex, east, 7);
	EXPECT_TRUE(reached_limit);
	EXPECT_TRUE(path.empty());
}

} // routing
} // marocco
