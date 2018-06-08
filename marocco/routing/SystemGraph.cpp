#include "marocco/routing/SystemGraph.h"
#include "marocco/Logger.h"
#include <unordered_set>

using namespace HMF::Coordinate;

namespace marocco {
namespace routing {

SystemGraph::SystemGraph(
	resource::HICANNManager const& mgr,
	pymarocco::PyMarocco const& pymarocco,
	routing_graph& graph) :
		mPyMarocco(pymarocco),
		mMgr(mgr),
		mGraph(graph),
		mWafers()
{
	std::unordered_set<Wafer> wafers;
	for (auto const& hicann : mMgr.present())
	{
		wafers.insert(hicann.toWafer());
	}

	for (auto const& wafer : wafers)
	{
		mWafers.insert(std::make_pair(wafer, WaferGraph(wafer, mgr, mPyMarocco, graph)));
	}

	// add wafer interconnects
}

size_t SystemGraph::numL1Busses() const
{
	return boost::num_vertices(getRoutingGraph());
}

routing_graph& SystemGraph::getRoutingGraph()
{
	return mGraph;
}

routing_graph const& SystemGraph::getRoutingGraph() const
{
	return mGraph;
}

} // namespace routing
} // namespace marocco
