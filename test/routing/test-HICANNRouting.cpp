#include "test/common.h"

#include <iostream>

#include "marocco/routing/WaferRouting.h"
#include "marocco/routing/HICANNRouting.h"
#include "pymarocco/PyMarocco.h"

using namespace HMF::Coordinate;

namespace {
auto const _pymarocco = pymarocco::PyMarocco::create();
}

namespace marocco {
namespace routing {

struct HICANNRoutingTest : public ::testing::Test
{
	virtual void SetupUp() {}
	virtual void TearDown() {}

	HICANNRoutingTest() :
		mMgr({}),
		mRoutingGraph(),
		mWaferGraph(HMF::Coordinate::Wafer(0), mMgr, *_pymarocco, mRoutingGraph)
	{}

	resource::HICANNManager mMgr;
	routing_graph mRoutingGraph;
	WaferGraph mWaferGraph;
};

TEST_F(HICANNRoutingTest, Base)
{
	routing_graph routingGraph;
	WaferGraph wg(HMF::Coordinate::Wafer(0), mMgr, *_pymarocco, routingGraph);

	std::vector<Route> route_list(42, Route({}, {}));
	LocalRoute lr(route_list.front(), Route::BusSegment(0));
}

} // routing
} // marocco
