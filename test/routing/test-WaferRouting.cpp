#include "test/common.h"

#include <cstdlib>
#include <iostream>
#include <type_traits>
#include <memory>

#include <boost/make_shared.hpp>

#include "marocco/graph.h"
#include "marocco/routing/WaferRoutingDijkstra.h"
#include "marocco/placement/Result.h"
#include "hal/HMFUtil.h"

#include "redman/backend/MockBackend.h"

using namespace HMF::Coordinate;

namespace {
auto const _pymarocco = pymarocco::PyMarocco::create();
sthal::System hw;
}

namespace marocco {
namespace routing {

class WaferRoutingTest :
	public ::testing::Test
{
public:
	typedef routing_graph::vertex_descriptor vertex_t;

	WaferRoutingTest() :
		mMgr(),
		mGraph(),
		mNpl(),
		mRoutingGraph(),
		mWaferGraph(),
		mSynLoss(),
		mWaferRouting(),
		mWaferId()
	{}

	virtual void SetUp()
	{
		// clear routing Graph before Test
		mRoutingGraph.clear();
		ASSERT_EQ(0, boost::num_vertices(mRoutingGraph));

		// build a clean new WaferGraph for some random wafer
		// FIXME: WaferRouting uses a hard-coded Wafer(0) coordinate
		mWaferId = Wafer(rand()%10000);

		auto backend = boost::make_shared<redman::backend::MockBackend>();

		// FIXME: revisit this when ctor of HICANNManager has been adapted
		mMgr.reset(new resource::HICANNManager(backend, {mWaferId}));

		ASSERT_EQ(384, mMgr->count_present());
		ASSERT_EQ(mWaferId, mMgr->begin_present()->toWafer());
		mGraph.reset(new marocco::graph_t());
		mNpl.reset(new marocco::placement::NeuronPlacementResult());
		mSynLoss.reset(new SynapseLoss(*mNpl, *mGraph));
		mWaferRouting.reset(
		    new WaferRoutingDijkstra(
		        mWaferId, mSynLoss, *_pymarocco, mRoutingGraph, *mGraph, hw, *mMgr));
		mWaferGraph = &mWaferRouting->mWaferGraph;
	}

	Route::Segments
	allocateRoute(
		Route::BusSegment const source,
		std::unordered_set<HICANNGlobal> targets,
		std::unordered_set<HICANNGlobal>& unreachable)
	{
		return mWaferRouting->allocateRoute(source, targets, unreachable);
	}

	HICANNGraph const& getHICANNGraph(HICANNGlobal const& h) const
	{
		if (mWaferGraph->mHICANN.size() != 384)
			throw std::runtime_error("invalid graph size");
		if (!(h.toWafer() == mWaferId))
			throw std::runtime_error("invalid coordinate");

		return mWaferGraph->mHICANN.at(h);
	}

	std::unique_ptr<resource::HICANNManager> mMgr;
	std::unique_ptr<marocco::graph_t> mGraph;
	std::unique_ptr<marocco::placement::NeuronPlacementResult> mNpl;
	routing_graph mRoutingGraph;
	WaferGraph const* mWaferGraph;
	boost::shared_ptr<SynapseLoss> mSynLoss;
	std::unique_ptr<WaferRouting> mWaferRouting;
	HMF::Coordinate::Wafer mWaferId;
};

TEST_F(WaferRoutingTest, WaferGraphDimensions)
{
	size_t const numHICANNs = 384;
	size_t const numVert = 256;
	size_t const numHor = 64;
	ASSERT_EQ(
		numHICANNs * (numVert + numHor),
		mWaferGraph->numL1Busses());
	ASSERT_LT(0, boost::num_edges(mRoutingGraph));
}

#define PROBE_L1_TOPOLOGY_TO(trg) \
	try { \
		vertex_t const _target = trg; \
		ASSERT_NE(source, _target); \
		\
		routing_graph::out_edge_iterator ei, eiend; \
		std::tie(ei, eiend) = boost::out_edges(source, mRoutingGraph); \
		bool found = false; \
		for (; ei != eiend; ++ei) \
		{ \
			if (boost::source(*ei, mRoutingGraph) == _target || \
				boost::target(*ei, mRoutingGraph) == _target ) \
				found = true; \
		} \
		ASSERT_TRUE(found) << hicann; \
	} catch (std::overflow_error const&) { \
	} catch (std::domain_error const&) {}

/** this test is supposed to check, wether the L1 topology witin a single Wafer
 *  is correctly represented.
 */
TEST_F(WaferRoutingTest, WaferGraphL1Connectivity)
{
	for (size_t ii = 0; ii<384; ++ii)
	{
		HICANNGlobal hicann{Enum(ii), mWaferId};
		HICANNGraph const& hg = getHICANNGraph(hicann);

		for (vertex_t const& source : hg.getVerticalBusses())
		{
			L1Bus const& l1bus = mRoutingGraph[source];
			ASSERT_EQ(L1Bus::Vertical, l1bus.getDirection());
			VLineOnHICANN const line(l1bus.getBusId());

			PROBE_L1_TOPOLOGY_TO(
				getHICANNGraph(hicann.south()).getVerticalBusses()[line.south()])
			PROBE_L1_TOPOLOGY_TO(
				getHICANNGraph(hicann.north()).getVerticalBusses()[line.north()])
		}

		for (vertex_t const& source: hg.getHorizontalBusses())
		{
			L1Bus const& l1bus = mRoutingGraph[source];
			ASSERT_EQ(L1Bus::Horizontal, l1bus.getDirection());
			HLineOnHICANN const line(l1bus.getBusId());

			PROBE_L1_TOPOLOGY_TO(
				getHICANNGraph(hicann.east()).getHorizontalBusses()[line.east()])
			PROBE_L1_TOPOLOGY_TO(
				getHICANNGraph(hicann.west()).getHorizontalBusses()[line.west()])
		}
	}
}

TEST_F(WaferRoutingTest, WaferGraphL1BusProperties)
{
	typedef std::tuple<
		std::set<HLineOnHICANN>,
		std::set<VLineOnHICANN> > tuple_t;
	std::map<HICANNGlobal, tuple_t> map;

	routing_graph::vertex_iterator vit, vitend;
	std::tie(vit, vitend) = boost::vertices(mRoutingGraph);
	for (; vit != vitend; ++vit)
	{
		L1Bus const& bus = mRoutingGraph[*vit];
		tuple_t& tuple = map[bus.hicann()];

		if (bus.getDirection() == L1Bus::Vertical) {
			std::get<1>(tuple).insert(VLineOnHICANN(bus.getBusId()));
		} else if (bus.getDirection() == L1Bus::Horizontal) {
			std::get<0>(tuple).insert(HLineOnHICANN(bus.getBusId()));
		} else {
			throw std::runtime_error("What!!");
		}
	}

	// now count if everything is there
	for (auto const& it : map)
	{
		ASSERT_EQ( 64, std::get<0>(it.second).size());
		ASSERT_EQ(256, std::get<1>(it.second).size());
	}
}

TEST_F(WaferRoutingTest, DISABLED_MazeRouting)
{
	ASSERT_LT(0, mWaferGraph->numL1Busses());
	size_t const max = mWaferGraph->numL1Busses()-1;

	for (size_t ii = 0; ii < 10; ++ii)
	{
		std::unordered_set<HICANNGlobal> targets;
		std::unordered_set<HICANNGlobal> unreachable;

		// choose a suitable random source
		vertex_t source= rand() % max;
		while (true)
		{
			L1Bus const& bus = mRoutingGraph[source];
			if (bus.getDirection() == L1Bus::Horizontal) {
				if (HLineOnHICANN(bus.getBusId()).toHRepeaterOnHICANN().isSending()) {
					break;
				}
			} else {
				ASSERT_ANY_THROW(allocateRoute(
					source, targets, unreachable));
			}
			source = rand() % max;
		}

		// add some random targets
		for (size_t jj=0; jj < (3 + size_t(rand())%3) || targets.size() == 0; ++jj)
		{
			HICANNGlobal target(Enum(rand() % 384), mWaferId);
			if(target == mRoutingGraph[source].hicann()) {
				std::cout << "skipping target" << std::endl;
				continue;
			}
			targets.insert(target);
		}

		// make sure, we have some targets
		EXPECT_LT(0, targets.size());

		// do the actual maze routing
		auto segments = allocateRoute(
			source, targets, unreachable);

		Route route(source, std::vector<HardwareProjection>());
		route.setSegments(std::move(segments));

		// check the route
		ASSERT_TRUE(route.length() > 0 || unreachable.size() == targets.size());
		ASSERT_TRUE(route.getSegments().size() > 0 || unreachable.size() == targets.size());

		// it might sometimes happen, that the routes could not be realized,
		// particularly in later iterations.
		EXPECT_LT(0, route.length()) << " iteration: " << ii;
		EXPECT_LT(0, route.getSegments().size()) << " iteration: " << ii;
	}
}

} // routing
} // marocco
