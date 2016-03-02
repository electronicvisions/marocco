#include "test/common.h"

#include "marocco/routing/L1Routing.h"
#include "marocco/routing/L1RoutingGraph.h"

using namespace HMF::Coordinate;

namespace marocco {
namespace routing {

TEST(L1Routing, toL1RouteWorksForSimpleTestCase)
{
	L1RoutingGraph rgraph;
	HICANNOnWafer hicann1(X(5), Y(5));
	HICANNOnWafer hicann2(X(6), Y(5));
	rgraph.add(hicann1);
	rgraph.add(hicann2);
	PathBundle::path_type path{rgraph[hicann1][HLineOnHICANN(46)],
	                           rgraph[hicann2][HLineOnHICANN(48)],
	                           rgraph[hicann2][VLineOnHICANN(39)]};
	auto route = toL1Route(rgraph.graph(), path);
	L1Route reference{HICANNOnWafer(X(5), Y(5)), HLineOnHICANN(46), HICANNOnWafer(X(6), Y(5)),
	                  HLineOnHICANN(48), VLineOnHICANN(39)};
	EXPECT_EQ(reference, route);
}

TEST(L1Routing, toL1RouteTreeWorksForSimpleTestCase)
{
	L1RoutingGraph rgraph;
	HICANNOnWafer hicann1(X(5), Y(5));
	HICANNOnWafer hicann2(X(6), Y(5));
	rgraph.add(hicann1);
	rgraph.add(hicann2);
	PathBundle bundle;
	bundle.add(
	    PathBundle::path_type{rgraph[hicann1][HLineOnHICANN(46)],
	                          rgraph[hicann2][HLineOnHICANN(48)],
	                          rgraph[hicann2][VLineOnHICANN(39)]});
	bundle.add(
	    PathBundle::path_type{rgraph[hicann1][HLineOnHICANN(46)],
	                          rgraph[hicann2][HLineOnHICANN(48)],
	                          rgraph[hicann2][VLineOnHICANN(7)]});

	auto tree = toL1RouteTree(rgraph.graph(), bundle);
	L1RouteTree reference;
	reference.add(
	    L1Route{HICANNOnWafer(X(5), Y(5)), HLineOnHICANN(46), HICANNOnWafer(X(6), Y(5)),
	            HLineOnHICANN(48), VLineOnHICANN(39)});
	reference.add(
	    L1Route{HICANNOnWafer(X(5), Y(5)), HLineOnHICANN(46), HICANNOnWafer(X(6), Y(5)),
	            HLineOnHICANN(48), VLineOnHICANN(7)});
	EXPECT_EQ(reference, tree);
}

} // namespace routing
} // namespace marocco
