#include "marocco/routing/WaferRoutingDijkstra.h"

#include <sstream>

#include "marocco/routing/WaferRoutingPriorityQueue.h"
#include "marocco/routing/SynapseLoss.h"
#include "marocco/routing/RoutingTargetVisitor.h"
#include "marocco/routing/WeightMap.h"
#include "marocco/routing/util.h"
#include "marocco/Logger.h"
#include "hal/Coordinate/iter_all.h"

using namespace HMF::Coordinate;

namespace marocco {
namespace routing {

WaferRoutingDijkstra::~WaferRoutingDijkstra()
{}

Route::Segments
WaferRoutingDijkstra::allocateRoute(
	Route::BusSegment const source,
	std::unordered_set<HICANNGlobal> targets,
	std::unordered_set<HICANNGlobal>& unreachable)
{
	auto const& routing_graph = getRoutingGraph();

	if (!unreachable.empty()) // we need an empty list
		throw std::runtime_error("unreachables must initially be empty");

	if (routing_graph[source].getDirection() != L1Bus::Horizontal) // we need an empty list
		throw std::runtime_error("source must be horizontal SPL1 repeater line");

	if (targets.empty()) // nothing todo
		return {};

	// TODO: a sparser hash map would be better here, because we are wasting a
	// lot of memory, num_vertices can be a rather large number.
	std::vector<int> distance(boost::num_vertices(routing_graph),
							  std::numeric_limits<int>::max());
	std::vector<int> predecessor(boost::num_vertices(routing_graph));

	// use disjksta with early abort by visitor if all targets have been reached
	// already
	std::unordered_map<HICANNGlobal, Route::BusSegment> lastMile;
	try {
		RoutingTargetVisitor rtv(predecessor, mUsage, targets, lastMile);
		boost::dijkstra_shortest_paths(
			routing_graph, source,
			boost::weight_map(mWeightMap)
				.distance_map(distance.data())
				.predecessor_map(predecessor.data())
				.visitor(rtv));
	} catch (RoutingEarlyAbort const& err) {
		// this means, we have already found a route to all targets sucessfully
	}

	// all targets not removed by visitor can be considered unreachable
	unreachable = targets;

	// collect all necesary bus segments representing this route by walking
	// backwards through the predecessor list.
	std::unordered_set<Route::BusSegment> buses;
	Route::Segments res;
	for (auto const target : lastMile)
	{
		std::vector<Route::BusSegment> _predecessor;
		Route::BusSegment cur = target.second;

		// insert all L1 segments into predecessor list
		// (for target to source, including the source segment)
		while (cur != source) {
			// remove all edges connecting this L1Bus vertex with any other
			// vertex, but leave it in the graph.
			clear_segment(cur);

			_predecessor.push_back(cur);
			buses.insert(cur);
			cur = predecessor.at(cur);
		}
		// push source bus as last element
		_predecessor.push_back(source);
		buses.insert(source);

		// insert pre
		res[target.first] = std::move(_predecessor);
	}


	// update weight map
	for (auto const entry : buses)
	{
		L1Bus const& l1bus = routing_graph[entry];
		if (l1bus.getDirection() == L1Bus::Horizontal) {
			mWeightMap.horizontal[l1bus.hicann().toHICANNOnWafer().id()] += 1;
		} else {
			mWeightMap.vertical[l1bus.hicann().toHICANNOnWafer().id()] += 1;
		}
	}

	return res;
}

} // namespace routing
} // namespace marocco
