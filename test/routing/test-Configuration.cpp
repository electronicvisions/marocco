#include "test/common.h"

#include "hal/Coordinate/iter_all.h"
#include "marocco/routing/Configuration.h"

using namespace HMF::Coordinate;

namespace marocco {
namespace routing {

TEST(RoutingConfiguration, configureWorksForSimpleRoute)
{
	L1Route route{HICANNOnWafer(X(5), Y(5)), DNCMergerOnHICANN(2), HLineOnHICANN(46),
	              HICANNOnWafer(X(6), Y(5)), HLineOnHICANN(48),    VLineOnHICANN(39)};

	auto hicann1 = boost::get<HICANNOnWafer>(route[0]);
	auto hicann2 = boost::get<HICANNOnWafer>(route[3]);

	sthal::Wafer hw;
	configure(hw, route);

	std::vector<HICANNOnWafer> expected{hicann1, hicann2};
	EXPECT_EQ(expected, hw.getAllocatedHicannCoordinates());

	sthal::HICANN hicann1_ref;
	sthal::HICANN hicann2_ref;
	hicann1_ref.repeater[HLineOnHICANN(46).toHRepeaterOnHICANN()].setOutput(right, true);
	hicann2_ref.repeater[HLineOnHICANN(48).toHRepeaterOnHICANN()].setForwarding(right);
	hicann2_ref.crossbar_switches.set(VLineOnHICANN(39), HLineOnHICANN(48), true);

	EXPECT_EQ(hicann1_ref, hw[hicann1]);
	EXPECT_EQ(hicann2_ref, hw[hicann2]);
}

TEST(RoutingConfiguration, configureWorksForSimpleTree)
{
	L1Route route{HICANNOnWafer(X(5), Y(5)), DNCMergerOnHICANN(2), HLineOnHICANN(46),
	              HICANNOnWafer(X(6), Y(5)), HLineOnHICANN(48),    VLineOnHICANN(39)};
	L1RouteTree tree(route);
	tree.add(
	    L1Route{HICANNOnWafer(X(5), Y(5)), DNCMergerOnHICANN(2), HLineOnHICANN(46),
	            VLineOnHICANN(8)});
	tree.add(L1Route{HICANNOnWafer(X(5), Y(5)), DNCMergerOnHICANN(2), HLineOnHICANN(46)});

	auto hicann1 = boost::get<HICANNOnWafer>(route[0]);
	auto hicann2 = boost::get<HICANNOnWafer>(route[3]);

	sthal::Wafer hw;
	configure(hw, tree);

	std::vector<HICANNOnWafer> expected{hicann1, hicann2};
	EXPECT_EQ(expected, hw.getAllocatedHicannCoordinates());

	sthal::HICANN hicann1_ref;
	sthal::HICANN hicann2_ref;
	hicann1_ref.repeater[HLineOnHICANN(46).toHRepeaterOnHICANN()].setOutput(right, true);
	hicann1_ref.crossbar_switches.set(VLineOnHICANN(8), HLineOnHICANN(46), true);
	hicann2_ref.repeater[HLineOnHICANN(48).toHRepeaterOnHICANN()].setForwarding(right);
	hicann2_ref.crossbar_switches.set(VLineOnHICANN(39), HLineOnHICANN(48), true);

	EXPECT_EQ(hicann1_ref, hw[hicann1]);
	EXPECT_EQ(hicann2_ref, hw[hicann2]);
}

TEST(RoutingConfiguration, configureWithTestOutput)
{
	HLineOnHICANN const hline(0);
	HICANNOnWafer const hicann1(Enum(92));
	HICANNOnWafer const hicann2(Enum(93));
	auto const repeater1 = hline.toHRepeaterOnHICANN();
	auto const repeater2 = hline.east().toHRepeaterOnHICANN();

	L1Route route{hicann1, repeater1.toRepeaterBlockOnHICANN(), hline, hicann2,
	              hline.east()};

	sthal::Wafer hw;
	configure(hw, route);

	std::vector<HICANNOnWafer> expected{hicann1, hicann2};
	EXPECT_EQ(expected, hw.getAllocatedHicannCoordinates());

	sthal::HICANN hicann1_ref;
	sthal::HICANN hicann2_ref;
	hicann1_ref.repeater[repeater1].setOutput(right);
	hicann2_ref.repeater[repeater2].setForwarding(right);

	EXPECT_EQ(hicann1_ref, hw[hicann1]);
	EXPECT_EQ(hicann2_ref, hw[hicann2]);
}

} // namespace routing
} // namespace marocco
