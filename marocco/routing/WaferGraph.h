#pragma once
#include <unordered_map>

#include "marocco/routing/routing_graph.h"
#include "marocco/routing/HICANNGraph.h"
#include "marocco/routing/Route.h"
#include "marocco/config.h"
#include "pymarocco/PyMarocco.h"
#include "hal/Coordinate/HMFGeometry.h"

namespace marocco {
namespace routing {

class WaferGraph
{
public:
	typedef HMF::Coordinate::VLineOnHICANN VLineOnHICANN;
	typedef HMF::Coordinate::HLineOnHICANN HLineOnHICANN;

	WaferGraph(HMF::Coordinate::Wafer const& wafer,
			   resource::HICANNManager const& mgr,
			   pymarocco::PyMarocco const& pymarocco,
			   routing_graph& graph);

	Route::BusSegment
	getSendingL1(HMF::Coordinate::HICANNGlobal const& h,
				 unsigned const outbuffer /*0-7*/) const;

	size_t numL1Busses() const;

private:
	routing_graph&       getRoutingGraph();
	routing_graph const& getRoutingGraph() const;

	HLineOnHICANN east(HLineOnHICANN const& hline) const;
	VLineOnHICANN south(VLineOnHICANN const& vline) const;

	static size_t getHorizontalSwap();
	static void setHorizontalSwap(size_t s);

	static size_t getVerticalSwap();
	static void setVerticalSwap(size_t s);

	std::unordered_map<HMF::Coordinate::HICANNGlobal, HICANNGraph> mHICANN;

	friend class WaferRoutingTest;

	pymarocco::PyMarocco const& mPyMarocco;
	HMF::Coordinate::Wafer mCoord;
	routing_graph& mGraph;
	resource::HICANNManager const& mMgr;

	static size_t horizontal_line_swap;
	static size_t vertical_line_swap;
};

} // namespace routing
} // namespace marocco
