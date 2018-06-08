#pragma once
#include <unordered_map>

#include "marocco/routing/routing_graph.h"
#include "marocco/routing/WaferGraph.h"
#include "marocco/routing/Route.h"
#include "marocco/config.h"
#include "pymarocco/PyMarocco.h"
#include "hal/Coordinate/HMFGeometry.h"

namespace marocco {
namespace routing {

class SystemGraph
{
public:
	SystemGraph(resource::HICANNManager const& mgr,
				pymarocco::PyMarocco const& pymarocco,
				routing_graph& graph);

	size_t numL1Busses() const;

private:
	routing_graph&       getRoutingGraph();
	routing_graph const& getRoutingGraph() const;

	friend class WaferRoutingTest;


	pymarocco::PyMarocco const& mPyMarocco;
	resource::HICANNManager const& mMgr;
	routing_graph& mGraph;

	std::unordered_map<HMF::Coordinate::Wafer, WaferGraph> mWafers;
};

} // namespace routing
} // namespace marocco
