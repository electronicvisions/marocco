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
		mWaferRouting.reset(new WaferRoutingDijkstra(mWaferId, mSynLoss, *_pymarocco,
		                                             mRoutingGraph, *mGraph, hw, *mMgr,
		                                             MPI::COMM_WORLD));
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

TEST_F(WaferRoutingTest, HICANNCoordinate)
{
	ASSERT_EQ(HICANNGlobal(Enum(0)), HICANNGlobal(Enum(0)));
	ASSERT_LT(HICANNGlobal(Enum(0), mWaferId),
			  HICANNGlobal(Enum(0), Wafer(mWaferId+1)));

	HICANNGlobal hicann(Enum(0), mWaferId);
	ASSERT_EQ(mWaferId, hicann.toWafer()) << mWaferId;

	for (size_t ii = 1; ii<384; ++ii) {
		ASSERT_FALSE(HICANNGlobal(Enum(ii)) == HICANNGlobal(Enum(0)));
	}
}

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

TEST(RoutingGraph, Traversal)
{
	typedef routing_graph::vertex_descriptor vertex_t;
	typedef routing_graph::edge_descriptor edge_t;

	routing_graph rg;

	std::vector<vertex_t>v = { boost::add_vertex(rg), boost::add_vertex(rg)};

	edge_t e;
	bool inserted;
	std::tie(e, inserted) = boost::add_edge(v[0], v[1], rg);
	ASSERT_TRUE(inserted);

	// NOTE: edge(0, 1) and edge(1, 0) are equal in an undirected graph (ug).
	// Which means, if we insert edge(0, 1) and use out_edges on vertex 1 we get
	// edge(0, 1), which in turn means if we call target(*out_edges(1, ug).first, ug)
	// the result is vertex 1 and not 0 as we would have expected. However,
	// there can be multiple edges between two vertices. For example, there can
	// be both edge(0, 1) and edge(0, 1) present at the same time.
	ASSERT_EQ(edge_t(0, 1, nullptr), edge_t(1, 0, nullptr));

	// make sure we reach vertex(1) from vertex(0)
	{
		routing_graph::out_edge_iterator it, itend;
		std::tie(it, itend) = boost::out_edges(v[0], rg);
		ASSERT_NE(it, itend);
		size_t cnt = 0;
		for(; it != itend; ++it)
		{
			ASSERT_EQ(*(it), e);
			vertex_t trg = boost::target(*(it), rg);
			ASSERT_EQ(v[1], trg);
			++cnt;
		}
		ASSERT_EQ(1, cnt);
	}

	// check targets from vertex(1), note that we are using boost::source rather
	// than boost::target. See NOTE above for explenation.
	{
		auto it = boost::out_edges(v[1], rg);
		ASSERT_NE(it.first, it.second);
		ASSERT_EQ(*(it.first), e);
		vertex_t src = boost::source(e, rg);
		ASSERT_EQ(v[0], src);
	}

	std::tie(e, inserted) = boost::add_edge(v[0], v[1], rg);
	ASSERT_TRUE(inserted);
	// there should no be edge(0, 1) and edge(1, 0) both be present in the graph
	ASSERT_EQ(2, boost::num_edges(rg));
}

} // routing
} // marocco
