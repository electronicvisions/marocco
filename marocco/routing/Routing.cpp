#include "marocco/routing/Routing.h"
#include "marocco/routing/WaferRoutingDijkstra.h"
#include "marocco/routing/WaferRoutingBackbone.h"
#include "marocco/routing/HICANNRouting.h"
#include "marocco/routing/SynapseLoss.h"
#include "marocco/Logger.h"

#include "tools/roqt/bindings/pyroqt.h"

#include <boost/make_shared.hpp>

namespace marocco {
namespace routing {

std::unique_ptr<typename Routing::result_type>
DefaultRouting::run(result_type const& placement_result)
{
	std::unique_ptr<Result> result(new Result);

	placement::Result const& placement = result_cast<placement::Result>(placement_result);

	mSynapseLoss = boost::make_shared<SynapseLoss>(placement.neuron_placement, getGraph());

	routing_graph routinggraph;

	auto const wafers = getManager().wafers();
	assert(wafers.size() == 1);
	auto const wafer = wafers.front();

	WaferRoutingBackbone global_router(wafer, mSynapseLoss, mPyMarocco,
		routinggraph, getGraph(), getHardware(),
		getManager());

	// WaferRoutingDijkstra global_router(wafer, mSynapseLoss, mPyMarocco,
	// 	routinggraph, getGraph(), getHardware(),
	// 	getManager());

	result->crossbar_routing = global_router.run(placement);

	HICANNRouting local_router(mSynapseLoss, mPyMarocco,
		getGraph(), getHardware(), getManager(),
		routinggraph);
	result->synapse_row_routing = local_router.run(placement, result->crossbar_routing);


	// write out the results for RoQt
	if (!mPyMarocco.roqt.empty()) {
		PyRoQt pyroqt(result->crossbar_routing, result->synapse_row_routing,
		              routinggraph);
		pyroqt.store(mPyMarocco.roqt);
	}

	return { std::move(result) };
}

boost::shared_ptr<SynapseLoss> DefaultRouting::getSynapseLoss() const
{
	return mSynapseLoss;
}

} // namespace routing
} // namespace marocco
