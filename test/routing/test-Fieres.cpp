#include <cstdlib>

#include "test/common.h"
#include "marocco/routing/Fieres.h"
#include "hal/Coordinate/iter_all.h"

using namespace HMF::Coordinate;

namespace marocco {
namespace routing {
namespace fieres {

class TestableAssignment : public Assignment {
public:
	TestableAssignment(SideHorizontal const& side) : Assignment(side) {}
	using Assignment::unassigned_p;
	using Assignment::assigned_p;
	using Assignment::mData;

	size_t test_count_unassigned() const {
		size_t unassigned = 0;
		for (auto const side_vertical : iter_all<SideVertical>()) {
			auto const& arr = mData[side_vertical];
			unassigned += std::count_if(arr.begin(), arr.end(), unassigned_p);
		}
		return unassigned;
	}

	template <typename UnaryP>
	void test_disable_all_but(UnaryP pred) {
		for (auto const drv : iter_all<SynapseDriverOnHICANN>()) {
			if (drv.toSideHorizontal() != mSide || pred(drv)) {
				continue;
			}

			add_defect(drv.toSideVertical(), drv.toSynapseDriverOnQuadrant());
		}
	}
};

class AssignmentTest : public ::testing::TestWithParam<SideHorizontal> {
public:
	AssignmentTest() : side(GetParam()), assignment(GetParam()) {}

	SideHorizontal side;
	TestableAssignment assignment;

	decltype(assignment.mData[top])& raw(SideVertical sidev) {
		return assignment.mData[sidev];
	}
};

TEST_P(AssignmentTest, AllowsRouteWhenEmpty) {
	EXPECT_TRUE(assignment.add(
	    InboundRoute(VLineOnHICANN(2), 5 /*drivers*/, 2 /*synapses*/, 5 /*assigned*/)));
}

TEST_P(AssignmentTest, HasSeveralChoicesForEachVLine) {
	InboundRoute const route{VLineOnHICANN(22), 1 /*drivers*/,
	                         1 /*synapses*/,    1 /*assigned*/};

	// We can reach 14 different synapse drivers from any given VLine
	size_t possibilities = route.line.toSynapseDriverOnHICANN(side).size();

	for (size_t ii = 0; ii < possibilities; ++ii) {
		EXPECT_TRUE(assignment.add(route));
	}

	// As every incoming route gets its own driver and the available drivers are limited.
	EXPECT_FALSE(assignment.add(route));

	auto const result = assignment.result();
	EXPECT_EQ(1, result.size()); // We only used one distinct VLineOnHICANN
	auto const driver_assignments = result.at(route.line);
	EXPECT_EQ(possibilities * route.assigned, driver_assignments.size());
}

TEST_P(AssignmentTest, DoesNotAssignDefectDrivers) {
	InboundRoute const route{VLineOnHICANN(22), 1 /*drivers*/,
	                         1 /*synapses*/,    1 /*assigned*/};

	// Disable all viable synapse drivers:
	auto const drivers = route.line.toSynapseDriverOnHICANN(side);
	for (auto const& drv : drivers) {
		assignment.add_defect(drv.toSideVertical(), drv.toSynapseDriverOnQuadrant());
	}

	EXPECT_FALSE(assignment.add(route));
	size_t count = 0;
	for (auto const side_vertical : iter_all<SideVertical>()) {
		for (auto const interval_ptr : raw(side_vertical)) {
			if (interval_ptr) {
				EXPECT_EQ(InboundRoute::DEFECT, interval_ptr->route.drivers);
				++count;
			}
		}
	}
	EXPECT_EQ(drivers.size(), count);
}

TEST_P(AssignmentTest, UsesSmallestPossibleGap) {
	InboundRoute const route{VLineOnHICANN(33), 1 /*drivers*/,
	                         1 /*synapses*/,    1 /*assigned*/};

	auto const drivers = route.line.toSynapseDriverOnHICANN(side);
	auto const large = drivers.at(2);
	auto const small = drivers.at(4);

	assignment.test_disable_all_but([&large, &small](SynapseDriverOnHICANN const& drv) {
		if (std::abs(int(drv.y()) - int(large.y())) < 4) {
			return true;
		}

		if (std::abs(int(drv.y()) - int(small.y())) < 2) {
			return true;
		}

		return false;
	});

	EXPECT_FALSE(!!raw(large.toSideVertical())[large.toSynapseDriverOnQuadrant()]);
	EXPECT_FALSE(!!raw(small.toSideVertical())[small.toSynapseDriverOnQuadrant()]);

	EXPECT_TRUE(assignment.add(route));

	EXPECT_FALSE(!!raw(large.toSideVertical())[large.toSynapseDriverOnQuadrant()]);
	EXPECT_TRUE(!!raw(small.toSideVertical())[small.toSynapseDriverOnQuadrant()]);
}

TEST_P(AssignmentTest, AchievesASnugFit) {
	InboundRoute const route{VLineOnHICANN(33), 3 /*drivers*/,
	                         1 /*synapses*/,    3 /*assigned*/};

	auto const drivers = route.line.toSynapseDriverOnHICANN(side);
	auto const target = drivers.at(2);

	// Disable all but 'target' and the next two synapse drivers.
	assignment.test_disable_all_but([&target](SynapseDriverOnHICANN const& drv) {
		return (drv.toSideVertical() == target.toSideVertical() &&
		        size_t(drv.toSynapseDriverOnQuadrant() -
		               target.toSynapseDriverOnQuadrant()) < 3);
	});

	// Check that we actually disabled the right ones.
	{
		EXPECT_EQ(3, assignment.test_count_unassigned());
		auto const side = target.toSideVertical();
		auto const onquadr = target.toSynapseDriverOnQuadrant();
		EXPECT_FALSE(raw(side)[SynapseDriverOnQuadrant(onquadr + 0)]);
		EXPECT_FALSE(raw(side)[SynapseDriverOnQuadrant(onquadr + 1)]);
		EXPECT_FALSE(raw(side)[SynapseDriverOnQuadrant(onquadr + 2)]);
	}

	// As there is a gap around a reachable synapse driver this should work:
	EXPECT_TRUE(assignment.add(route));

	// Now all drivers should be 'assigned'.
	EXPECT_EQ(0, assignment.test_count_unassigned());

	// Check the interval where our route was entered:
	{
		auto const onquadr = target.toSynapseDriverOnQuadrant();
		auto const ival = *raw(target.toSideVertical())[onquadr];
		EXPECT_EQ(route.line, ival.route.line);
		EXPECT_EQ(onquadr, ival.primary);
		EXPECT_EQ(onquadr, ival.begin);
		EXPECT_EQ(onquadr + 2 + 1, ival.end); // +1: off-the-end pointer
	}

	// Check that the three drivers were assigned to the vline.
	{
		auto const result = assignment.result();
		EXPECT_EQ(1, result.size());
		auto const driver_assignments = result.at(route.line);
		EXPECT_EQ(1, driver_assignments.size());
		auto const& da = driver_assignments.at(0);
		EXPECT_EQ(target, da.primary);
		EXPECT_EQ(route.assigned, da.drivers.size());
	}
}

TEST_P(AssignmentTest, MayClipARoute) {
	InboundRoute const route{VLineOnHICANN(33), 8 /*drivers*/,
	                         1 /*synapses*/,    8 /*assigned*/};

	auto const drivers = route.line.toSynapseDriverOnHICANN(side);
	auto const target = drivers.at(2);

	// Disable all but 'target' and the two adjacent synapse drivers.
	assignment.test_disable_all_but([&target](SynapseDriverOnHICANN const& drv) {
		return (drv.toSideVertical() == target.toSideVertical() &&
		        std::abs(int(drv.toSynapseDriverOnQuadrant()) -
		                 int(target.toSynapseDriverOnQuadrant())) <= 1);
	});

	// As there is no space to fit in eight (route.assigned) drivers
	// this will be clipped to the three available drivers (but still work).
	EXPECT_TRUE(assignment.add(route));

	// Check that (only) the three drivers were assigned to the vline.
	{
		auto const result = assignment.result();
		EXPECT_EQ(1, result.size());
		auto const driver_assignments = result.at(route.line);
		EXPECT_EQ(1, driver_assignments.size());
		auto const& da = driver_assignments.at(0);
		EXPECT_EQ(target, da.primary);
		EXPECT_EQ(3, da.drivers.size());
	}
}


INSTANTIATE_TEST_CASE_P(AssignmentTestSides, AssignmentTest,
                        ::testing::Values(left, right));

TEST(Fieres, Issue1666_Case1) {
	typedef std::vector<DriverInterval> IntervalList;
	IntervalList list = {
		DriverInterval(VLineOnHICANN(133), 2, 40),
		DriverInterval(VLineOnHICANN(172), 7, 688),
		DriverInterval(VLineOnHICANN(238), 8, 648),
		DriverInterval(VLineOnHICANN(208), 7, 674),
		DriverInterval(VLineOnHICANN(175), 6, 600),
		DriverInterval(VLineOnHICANN(135), 1, 16),
		DriverInterval(VLineOnHICANN(163), 1, 24),
		DriverInterval(VLineOnHICANN(142), 7, 632),
		DriverInterval(VLineOnHICANN(169), 6, 648),
		DriverInterval(VLineOnHICANN(234), 7, 660),
		DriverInterval(VLineOnHICANN(235), 7, 630),
		DriverInterval(VLineOnHICANN(143), 6, 632),
		DriverInterval(VLineOnHICANN(177), 7, 606),
		DriverInterval(VLineOnHICANN(209), 8, 630),
		DriverInterval(VLineOnHICANN(205), 3, 321),
		DriverInterval(VLineOnHICANN(203), 4, 325)
	};

	Fieres fieres(list, HMF::Coordinate::right);

	size_t const drivers_requested = std::accumulate(list.begin(), list.end(), 0,
			[](size_t cnt, DriverInterval const& entry) {
			return cnt + entry.driver;
			});

	auto result = fieres.result(HICANNGlobal(Enum(0)));

	size_t drivers_assigned = 0;
	for (auto const& vline : result) {
		for (auto const& as : vline.second) {
			drivers_assigned +=as.drivers.size();
		}
	}

	ASSERT_EQ(drivers_requested, drivers_assigned);
}

TEST(Fieres, Issue1666_Case2) {
	typedef std::vector<DriverInterval> IntervalList;
	IntervalList list = {
		DriverInterval(VLineOnHICANN(142), 6, 616),
		DriverInterval(VLineOnHICANN(226), 2, 38),
		DriverInterval(VLineOnHICANN(236), 8, 584),
		DriverInterval(VLineOnHICANN(207), 6, 618),
		DriverInterval(VLineOnHICANN(177), 7, 590),
		DriverInterval(VLineOnHICANN(176), 6, 612),
		DriverInterval(VLineOnHICANN(228), 2, 35),
		DriverInterval(VLineOnHICANN(237), 7, 648),
		DriverInterval(VLineOnHICANN(137), 8, 642),
		DriverInterval(VLineOnHICANN(178), 7, 634),
		DriverInterval(VLineOnHICANN(212), 7, 634),
		DriverInterval(VLineOnHICANN(146), 7, 576),
		DriverInterval(VLineOnHICANN(145), 7, 590),
		DriverInterval(VLineOnHICANN(144), 4, 337),
		DriverInterval(VLineOnHICANN(174), 4, 327),
		DriverInterval(VLineOnHICANN(202), 4, 290),
		DriverInterval(VLineOnHICANN(200), 4, 332)
	};

	Fieres fieres(list, HMF::Coordinate::right);

	size_t const drivers_requested = std::accumulate(list.begin(), list.end(), 0,
			[](size_t cnt, DriverInterval const& entry) {
			return cnt + entry.driver;
			});

	auto result = fieres.result(HICANNGlobal(Enum(0)));

	size_t drivers_assigned = 0;
	for (auto const& vline : result) {
		for (auto const& as : vline.second) {
			drivers_assigned +=as.drivers.size();
		}
	}

	ASSERT_EQ(drivers_requested, drivers_assigned);
}

TEST(Fieres, Issue1666_Case3) {
	typedef std::vector<DriverInterval> IntervalList;
	IntervalList list = {
		DriverInterval(VLineOnHICANN(47), 7, 624),
		DriverInterval(VLineOnHICANN(63), 2, 40),
		DriverInterval(VLineOnHICANN(49), 7, 634),
		DriverInterval(VLineOnHICANN(16), 7, 690),
		DriverInterval(VLineOnHICANN(20), 6, 614),
		DriverInterval(VLineOnHICANN(51), 8, 692),
		DriverInterval(VLineOnHICANN(109), 8, 698),
		DriverInterval(VLineOnHICANN(107), 6, 634),
		DriverInterval(VLineOnHICANN(10), 8, 674),
		DriverInterval(VLineOnHICANN(108), 7, 626),
		DriverInterval(VLineOnHICANN(45), 4, 330),
		DriverInterval(VLineOnHICANN(111), 4, 315)
	};

	Fieres fieres(list, HMF::Coordinate::left);

	size_t const drivers_requested = std::accumulate(list.begin(), list.end(), 0,
			[](size_t cnt, DriverInterval const& entry) {
			return cnt + entry.driver;
			});

	auto result = fieres.result(HICANNGlobal(Enum(0)));

	size_t drivers_assigned = 0;
	for (auto const& vline : result) {
		for (auto const& as : vline.second) {
			drivers_assigned +=as.drivers.size();
		}
	}

	ASSERT_EQ(drivers_requested, drivers_assigned);
}

} // namespace fieres
} // namespace routing
} // namespace marocco
