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

	decltype(assignment.mData[0]) & raw(SideVertical sidev) {
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
		EXPECT_FALSE(raw(side)[onquadr + 0]);
		EXPECT_FALSE(raw(side)[onquadr + 1]);
		EXPECT_FALSE(raw(side)[onquadr + 2]);
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

} // namespace fieres
} // namespace routing
} // namespace marocco
