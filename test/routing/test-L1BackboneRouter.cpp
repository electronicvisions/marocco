#include "test/common.h"

#include <iostream>
#include <algorithm>
#include <iterator>

#include "hal/Coordinate/HICANN.h"
#include "hal/Coordinate/iter_all.h"
#include "marocco/Logger.h"
#include "marocco/routing/Configuration.h"
#include "marocco/routing/L1BackboneRouter.h"
#include "marocco/routing/L1Routing.h"

using namespace HMF::Coordinate;

namespace marocco {
namespace routing {

class AL1BackboneRouter : public ::testing::Test
{
protected:
	static void SetUpTestCase()
	{
		for (auto hicann : iter_all<HICANNOnWafer>()) {
			routing_graph.add(hicann);
		}
	}

	static L1RoutingGraph routing_graph;
}; // AL1BackboneRouter

L1RoutingGraph AL1BackboneRouter::routing_graph;

TEST_F(AL1BackboneRouter, determinesPathToTargets)
{
	HICANNOnWafer hicann(X(5), Y(5));
	auto hline = SendingRepeaterOnHICANN(3).toHLineOnHICANN();
	auto const& graph = routing_graph.graph();

	L1GraphWalker walker(graph);
	L1BackboneRouter backbone(walker, routing_graph[hicann][hline]);
	std::vector<HICANNOnWafer> targets = {HICANNOnWafer(X(4), Y(4)), HICANNOnWafer(X(6), Y(6)),
	                                      HICANNOnWafer(X(12), Y(7)), HICANNOnWafer(X(13), Y(5))};

	for (auto const& target : targets) {
		backbone.add_target(target);
	}

	backbone.run();

	EXPECT_TRUE(backbone.path_to(HICANNOnWafer(X(11), Y(11))).empty());

	for (auto const& target : targets) {
		auto path = backbone.path_to(target);
		EXPECT_FALSE(path.empty()) << target;
	}

	VLineOnHICANN vline;
	L1BusOnWafer bus;
	PathBundle::path_type path, reference;
	HICANNOnWafer target;

	target = HICANNOnWafer(X(6), Y(6));
	path = backbone.path_to(target);
	ASSERT_EQ(4, path.size());
	std::reverse(path.begin(), path.end());
	EXPECT_EQ(L1BusOnWafer(hicann, hline), graph[path.back()]);
	path.pop_back();
	EXPECT_EQ(L1BusOnWafer(hicann.east(), hline.east()), graph[path.back()]);
	path.pop_back();
	bus = graph[path.back()];
	EXPECT_EQ(hicann.east(), bus.toHICANNOnWafer());
	ASSERT_TRUE(bus.is_vertical());
	vline = bus.toVLineOnHICANN();
	path.pop_back();
	bus = graph[path.back()];
	EXPECT_EQ(hicann.east().south(), bus.toHICANNOnWafer());
	ASSERT_TRUE(bus.is_vertical());
	EXPECT_EQ(vline.south(), bus.toVLineOnHICANN());
	path.pop_back();
	EXPECT_TRUE(path.empty());

	target = HICANNOnWafer(X(4), Y(4));
	path = backbone.path_to(target);
	ASSERT_EQ(4, path.size());
	std::reverse(path.begin(), path.end());
	EXPECT_EQ(L1BusOnWafer(hicann, hline), graph[path.back()]);
	path.pop_back();
	EXPECT_EQ(L1BusOnWafer(hicann.west(), hline.west()), graph[path.back()]);
	path.pop_back();
	bus = graph[path.back()];
	EXPECT_EQ(hicann.west(), bus.toHICANNOnWafer());
	ASSERT_TRUE(bus.is_vertical());
	vline = bus.toVLineOnHICANN();
	path.pop_back();
	bus = graph[path.back()];
	EXPECT_EQ(hicann.west().north(), bus.toHICANNOnWafer());
	ASSERT_TRUE(bus.is_vertical());
	EXPECT_EQ(vline.north(), bus.toVLineOnHICANN());
	path.pop_back();
	EXPECT_TRUE(path.empty());
}

TEST_F(AL1BackboneRouter, determinesPathToTargetUsingDetour)
{
	HICANNOnWafer hicann(X(5), Y(5));
	HICANNOnWafer target = hicann.east();
	auto hline = SendingRepeaterOnHICANN(3).toHLineOnHICANN();

	L1RoutingGraph restricted_routing_graph = routing_graph;
	restricted_routing_graph.remove(hicann.east(), hline.east());
	auto const& graph = restricted_routing_graph.graph();

	L1GraphWalker walker(graph);
	L1BackboneRouter backbone(walker, restricted_routing_graph[hicann][hline]);
	backbone.add_target(target);

	L1BusOnWafer bus;
	VLineOnHICANN vline;
	HICANNOnWafer detour_hicann;
	HLineOnHICANN detour_hline;

	backbone.run();
	auto path = backbone.path_to(target);
	ASSERT_EQ(7, path.size());
	std::reverse(path.begin(), path.end());
	EXPECT_EQ(L1BusOnWafer(hicann, hline), graph[path.back()]);
	path.pop_back();
	bus = graph[path.back()];
	EXPECT_EQ(hicann, bus.toHICANNOnWafer());
	ASSERT_TRUE(bus.is_vertical());
	vline = bus.toVLineOnHICANN();
	path.pop_back();
	bus = graph[path.back()];
	detour_hicann = bus.toHICANNOnWafer();
	EXPECT_TRUE(detour_hicann == hicann.north() || detour_hicann == hicann.south())
	    << detour_hicann;
	EXPECT_TRUE(bus.is_vertical());
	path.pop_back();
	bus = graph[path.back()];
	EXPECT_EQ(detour_hicann, bus.toHICANNOnWafer());
	ASSERT_TRUE(bus.is_horizontal());
	detour_hline = bus.toHLineOnHICANN();
	path.pop_back();
	EXPECT_EQ(L1BusOnWafer(detour_hicann.east(), detour_hline.east()), graph[path.back()]);
	path.pop_back();
	bus = graph[path.back()];
	EXPECT_EQ(detour_hicann.east(), bus.toHICANNOnWafer());
	ASSERT_TRUE(bus.is_vertical());
	path.pop_back();
	bus = graph[path.back()];
	EXPECT_EQ(hicann.east(), bus.toHICANNOnWafer());
	ASSERT_TRUE(bus.is_vertical());
	path.pop_back();
	EXPECT_TRUE(path.empty());
}

TEST_F(AL1BackboneRouter, usesMultipleDetoursIfNecessary)
{
	// Because the wafer is non-rectangular, two detours may be necessary.
	// BV: “A typical testcase arises if we assume that the two reticles which have no
	// High-Speed Connection are disabled […] and e.g. want to route from reticle 27 to
	// 32, which can be achieved by taking 2 detours.”
	// (cf. https://brainscales-r.kip.uni-heidelberg.de/projects/symap2ic/wiki/Coordinates)

	L1RoutingGraph restricted_routing_graph;
	for (auto hicann : iter_all<HICANNOnWafer>()) {
		if (hicann.toDNCOnWafer().toEnum() == 19 || hicann.toDNCOnWafer().toEnum() == 28) {
			continue;
		}
		restricted_routing_graph.add(hicann);
	}
	auto const& graph = restricted_routing_graph.graph();

	HICANNOnWafer const source(X(12), Y(8));
	ASSERT_EQ(27, source.toDNCOnWafer().toEnum());
	HICANNOnWafer const target(X(35), Y(8));
	ASSERT_EQ(32, target.toDNCOnWafer().toEnum());
	auto const hline = SendingRepeaterOnHICANN(3).toHLineOnHICANN();

	L1GraphWalker walker(graph);
	L1BackboneRouter backbone(walker, restricted_routing_graph[source][hline]);
	backbone.add_target(target);
	backbone.run();

	auto const path = backbone.path_to(target);
	ASSERT_FALSE(path.empty());

	bool detour = false;
	std::vector<L1BusOnWafer> turns;
	for (L1RoutingGraph::vertex_descriptor const vertex : path) {
		auto const& bus = graph[vertex];
		if (detour == false && bus.is_vertical()) {
			detour = true;
		} else if (detour == true && bus.is_horizontal()) {
			detour = false;
			turns.push_back(bus);
		}
	}
	ASSERT_FALSE(turns.empty());
	EXPECT_EQ(2, turns.size());
	{
		auto const dnc = turns.front().toHICANNOnWafer().toDNCOnWafer().toEnum();
		if (dnc != 10 && dnc != 35) {
			FAIL() << "detour uses unexpected reticle " << dnc;
		}
	}

	{
		auto const dnc = turns.back().toHICANNOnWafer().toDNCOnWafer().toEnum();
		if (!(20 <= dnc && dnc <= 22) && !(29 <= dnc && dnc <= 31)) {
			FAIL() << "detour uses unexpected reticle " << dnc;
		}
	}
}

TEST_F(AL1BackboneRouter, acceptsScoringFunction)
{
	HICANNOnWafer hicann(X(5), Y(5));
	HICANNOnWafer target = hicann.north();
	auto hline = SendingRepeaterOnHICANN(3).toHLineOnHICANN();

	// There are 8 possible vertical lines that can be used to get to the northern HICANN.
	// To demonstrate the scoring, we will explicitly choose one out of them.

	auto vlines = hline.toVLineOnHICANN();
	auto const& graph = routing_graph.graph();
	L1GraphWalker walker(graph);
	for (auto const& magic_vline : vlines) {
		L1BackboneRouter backbone(
			walker, routing_graph[hicann][hline],
			[&graph, target,
			 magic_vline](L1RoutingGraph::vertex_descriptor const& vertex) -> size_t {
				return graph[vertex] == L1BusOnWafer(target, magic_vline.north()) ? 42 : 1;
			});
		backbone.add_target(target);
		backbone.run();

		auto path = backbone.path_to(target);
		ASSERT_EQ(3, path.size());
		std::reverse(path.begin(), path.end());
		EXPECT_EQ(L1BusOnWafer(hicann, hline), graph[path.back()]);
		path.pop_back();
		EXPECT_EQ(L1BusOnWafer(hicann, magic_vline), graph[path.back()]);
		path.pop_back();
		EXPECT_EQ(L1BusOnWafer(hicann.north(), magic_vline.north()), graph[path.back()]);
		path.pop_back();
		EXPECT_TRUE(path.empty());
	}
}

TEST_F(AL1BackboneRouter, isNotConfusedWhenTargetEqSource)
{
	HICANNOnWafer hicann(X(5), Y(5));
	auto hline = SendingRepeaterOnHICANN(3).toHLineOnHICANN();

	auto const& graph = routing_graph.graph();
	L1GraphWalker walker(graph);
	L1BackboneRouter backbone(walker, routing_graph[hicann][hline]);
	backbone.add_target(hicann);
	backbone.run();
	auto path = backbone.path_to(hicann);
	ASSERT_EQ(2, path.size());
	EXPECT_EQ(hicann, graph[path.front()].toHICANNOnWafer());
	EXPECT_EQ(hicann, graph[path.back()].toHICANNOnWafer());
	EXPECT_TRUE(graph[path.front()].is_horizontal());
	EXPECT_TRUE(graph[path.back()].is_vertical());
}

TEST_F(AL1BackboneRouter, doesNotCreateCircularPredecessorMapWhenNearEdgeOfWafer)
{
	// Routing from {13, 1} to [12, 0] and [11, 2]:
	//                      [12, 0]   (13, 0)   (14, 0)
	//                         ^
	//                      (12, 1) < {13, 1}   (14, 1)
	//                         v
	//  (10, 2)   [11, 2] < (12, 2)   (13, 2)   (14, 2)
	// Due to the wafer edge, a detour has to be taken to reach (11, 2).  Note that there
	// is a target HICANN in the same column where the detour has to be taken.
	HICANNOnWafer const source(X(13), Y(1));
	auto hline = SendingRepeaterOnHICANN(3).toHLineOnHICANN();

	std::vector<HICANNOnWafer> targets = {HICANNOnWafer(X(11), Y(2)), HICANNOnWafer(X(12), Y(0))};

	auto const& graph = routing_graph.graph();

	L1GraphWalker walker(graph);
	L1BackboneRouter backbone(walker, routing_graph[source][hline]);

	for (auto const& target : targets) {
		backbone.add_target(target);
	}

	backbone.run();

	for (auto const& target : targets) {
		auto path = backbone.path_to(target);
		EXPECT_FALSE(path.empty()) << target;
	}
}

TEST_F(AL1BackboneRouter, use_only_one_switch_per_hline_right)
{
	/**
	 * Routing from  [2] to its both neighbours
	 * Difficulty:
	 * 		do not feed both neighbours from the sending hicann.
	 *
	 * 		  S   < S >   S
	 * 		      +-*-+
	 * 		  S   | S |   S
	 *
	 * 	Not allowed configuration, when the signal is routed from *
	 * 	to two VLines on the same HICANN to feed the neighbouring Synapses
	 *
	 **/
	HICANNOnWafer const source_hicann(X(21), Y(0));
	HICANNOnWafer const target1_hicann(X(20), Y(0));
	HICANNOnWafer const target2_hicann(X(22), Y(0));

	auto hline = SendingRepeaterOnHICANN(3).toHLineOnHICANN();

	auto const& graph = routing_graph.graph();

	auto source = routing_graph[source_hicann][hline];
	std::vector<HICANNOnWafer> targets = {target1_hicann, target2_hicann};

	L1GraphWalker walker(graph);
	L1BackboneRouter backbone(walker, source);

	for (auto const& target : targets) {
		backbone.add_target(target);
	}

	backbone.run();

	sthal::Wafer wafer;

	auto path1 = backbone.path_to(targets[0]);
	auto path2 = backbone.path_to(targets[1]);

	EXPECT_EQ(path1.front(), source);
	EXPECT_EQ(path1.front(), path2.front());

	for (auto segment : path1) {
		MAROCCO_TRACE(graph[segment]);
	}
	MAROCCO_TRACE("");
	for (auto segment : path2) {
		MAROCCO_TRACE(graph[segment]);
	}

	EXPECT_FALSE(
	    graph[path1[1]].toHICANNOnWafer() ==
	    graph[path2[1]]
	        .toHICANNOnWafer()); // it is not allowed to go from the source-HLine to 2 VLines

	for (auto const& target : targets) {
		auto path = backbone.path_to(target);
		EXPECT_FALSE(path.empty()) << target;
		EXPECT_EQ(source, path.front()); // check for cycle

		configure(wafer, toL1Route(graph, path));
	}

	for (auto hc : wafer.getAllocatedHicannCoordinates()) {
		EXPECT_NO_THROW(wafer[hc].crossbar_switches.check_exclusiveness(1, 1, std::cerr)) << hc;
	}
}

TEST_F(AL1BackboneRouter, use_only_one_switch_per_hline_left)
{
	/**
	 * Routing from  [2] to its both neighbours
	 * Difficulty:
	 * 		do not feed both neighbours from the sending hicann.
	 *
	 * 		  S   < S >   S
	 * 		      +-*-+
	 * 		  S   | S |   S
	 *
	 * 	Not allowed configuration, when the signal is routed from *
	 * 	to two VLines on the same HICANN to feed the neighbouring Synapses
	 *
	 **/
	HICANNOnWafer const source_hicann(X(14), Y(0));
	HICANNOnWafer const target1_hicann(X(13), Y(0));
	HICANNOnWafer const target2_hicann(X(15), Y(0));

	auto hline = SendingRepeaterOnHICANN(3).toHLineOnHICANN();

	auto const& graph = routing_graph.graph();

	auto source = routing_graph[source_hicann][hline];
	std::vector<HICANNOnWafer> targets = {target1_hicann, target2_hicann};

	L1GraphWalker walker(graph);
	L1BackboneRouter backbone(walker, source);

	for (auto const& target : targets) {
		backbone.add_target(target);
	}

	backbone.run();

	sthal::Wafer wafer;

	auto path1 = backbone.path_to(targets[0]);
	auto path2 = backbone.path_to(targets[1]);

	EXPECT_EQ(path1.front(), source);
	EXPECT_EQ(path1.front(), path2.front());

	for (auto segment : path1) {
		MAROCCO_TRACE(graph[segment]);
	}
	MAROCCO_TRACE("");
	for (auto segment : path2) {
		MAROCCO_TRACE(graph[segment]);
	}

	EXPECT_FALSE(
	    graph[path1[1]].toHICANNOnWafer() ==
	    graph[path2[1]]
	        .toHICANNOnWafer()); // it is not allowed to go from the source-HLine to 2 VLines

	for (auto const& target : targets) {
		auto path = backbone.path_to(target);
		EXPECT_FALSE(path.empty()) << target;
		EXPECT_EQ(source, path.front()); // check for cycle

		configure(wafer, toL1Route(graph, path));
	}

	for (auto hc : wafer.getAllocatedHicannCoordinates()) {
		EXPECT_NO_THROW(wafer[hc].crossbar_switches.check_exclusiveness(1, 1, std::cerr)) << hc;
	}
}

TEST_F(AL1BackboneRouter, noCyclesWhenSourceTargetAndDetourInTheSameColumn)
{
	// Routing from {12, 1} to [12, 0] and [11, 2]:
	//                      [12, 0]   (13, 0)
	//                         ^
	//                      {12, 1}   (13, 1)
	//                         v
	//  (10, 2)   [11, 2] < (12, 2)   (13, 2)
	// Due to the wafer edge, a detour has to be taken to reach (11, 2). Note
	// that the column where the detour has to be taken, contains the source
	// and a target HICANN.
	HICANNOnWafer const source_hicann(X(12), Y(1));
	auto hline = SendingRepeaterOnHICANN(3).toHLineOnHICANN();
	auto source = routing_graph[source_hicann][hline];

	std::vector<HICANNOnWafer> targets = {HICANNOnWafer(X(11), Y(2)), HICANNOnWafer(X(12), Y(0))};

	auto const& graph = routing_graph.graph();

	L1GraphWalker walker(graph);
	L1BackboneRouter backbone(walker, source);

	for (auto const& target : targets) {
		backbone.add_target(target);
	}

	backbone.run();

	for (auto const& target : targets) {
		auto path = backbone.path_to(target);
		EXPECT_FALSE(path.empty()) << target;
		EXPECT_EQ(source, path.front()); // check for cycle
	}
}

TEST_F(AL1BackboneRouter, validConfigWhenTargetAndDetourInDefectiveColumn)
{
	// Routing from {14, 0} to [12, 1] and [13, 3]:
	// ¦12, 0¦   (13, 0) < {14, 0}
	//              v
	// [12, 1] < (13, 1)   (14, 1)
	//              v
	// (12, 2)   ¦13, 2¦   (14, 2)
	//              v
	// (12, 3)   [13, 3]   (14, 3)
	// Defects:
	//  - on ¦12, 0¦ the hline connected to the source l1 bus
	//  - on ¦13, 2¦ 7 of 8 vlines connected to the source l1 bus via (13, 0)
	// Difficulty:
	// A detour is needed to reach HICANN [12, 1], and in the column of the
	// detour there are defective vlines, so that there is only one vertical
	// route in that column to reach the other target HICANN.
	// Test:
	// 1.) The router must not connect one hline to several vlines on a HICANN (mandatory)
	// 2.) Both targets should be reached (optionally).
	HICANNOnWafer const source_hicann(X(14), Y(0));
	auto hline = SendingRepeaterOnHICANN(3).toHLineOnHICANN();

	L1RoutingGraph restricted_routing_graph = routing_graph;
	restricted_routing_graph.remove(source_hicann.west().west(), hline.west().west());

	auto vlines = hline.west().toVLineOnHICANN();
	ASSERT_EQ(vlines.size(), 8);
	// remove all but last vline from HICANN (13, 2)
	for (size_t i = 0; i < 7; ++i) {
		restricted_routing_graph.remove(
		    source_hicann.west().south().south(), vlines[i].south().south());
	}

	auto const& graph = restricted_routing_graph.graph();

	auto source = routing_graph[source_hicann][hline];
	std::vector<HICANNOnWafer> targets = {HICANNOnWafer(X(12), Y(1)), HICANNOnWafer(X(13), Y(3))};

	L1GraphWalker walker(graph);
	L1BackboneRouter backbone(walker, source);

	for (auto const& target : targets) {
		backbone.add_target(target);
	}

	backbone.run();

	sthal::Wafer wafer;

	// TODO: we currently check that both targets are reached. This might be attenuated to check
	// that at least one target is reached.
	for (auto const& target : targets) {
		auto path = backbone.path_to(target);
		EXPECT_FALSE(path.empty()) << target;
		EXPECT_EQ(source, path.front()); // check for cycle

		configure(wafer, toL1Route(graph, path));
	}

	for (auto hc : wafer.getAllocatedHicannCoordinates()) {
		EXPECT_NO_THROW(wafer[hc].crossbar_switches.check_exclusiveness(1,1, std::cerr)) << hc;
	}
}

} // routing
} // marocco
