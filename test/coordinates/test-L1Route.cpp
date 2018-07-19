#include <gtest/gtest.h>

#include "hal/Coordinate/HMFGeometry.h"
#include "marocco/coordinates/L1Route.h"

namespace marocco {

using namespace HMF::Coordinate;

class ASimpleL1Route : public ::testing::Test
{
protected:
	L1Route route{HICANNOnWafer(X(5), Y(5)),
	              Merger0OnHICANN(2),
	              DNCMergerOnHICANN(2),
	              HLineOnHICANN(46),
	              HICANNOnWafer(X(6), Y(5)),
	              HLineOnHICANN(48),
	              VLineOnHICANN(39),
	              SynapseDriverOnHICANN(left, Y(99)),
	              SynapseOnHICANN(SynapseColumnOnHICANN(5), SynapseRowOnHICANN(0))};
};

TEST(L1Route, empty)
{
	L1Route empty_route;
	ASSERT_TRUE(empty_route.empty());
}

TEST_F(ASimpleL1Route, append)
{
	L1Route other;
	ASSERT_ANY_THROW(other.append(HICANNOnWafer(X(5), Y(5))));
	ASSERT_ANY_THROW(other.append(HICANNOnWafer(X(5), Y(5)), HICANNOnWafer(X(5), Y(6))));
	ASSERT_NO_THROW(other.append(HICANNOnWafer(X(5), Y(5)), Merger0OnHICANN(2)));
	ASSERT_ANY_THROW(other.append(HLineOnHICANN(42)));
	ASSERT_ANY_THROW(other.append(HICANNOnWafer(X(6), Y(5)), HLineOnHICANN(48)));
	ASSERT_NO_THROW(other.append(DNCMergerOnHICANN(2)));
	ASSERT_NO_THROW(other.append(HLineOnHICANN(46)));
	ASSERT_ANY_THROW(other.append(HLineOnHICANN(42)));
	ASSERT_ANY_THROW(other.append(VLineOnHICANN(39)));
	ASSERT_ANY_THROW(other.append(HICANNOnWafer(X(6), Y(5)), HLineOnHICANN(42)));
	ASSERT_NO_THROW(other.append(HICANNOnWafer(X(6), Y(5)), HLineOnHICANN(48)));
	ASSERT_NO_THROW(other.append(VLineOnHICANN(39)));
	ASSERT_NO_THROW(other.append(SynapseDriverOnHICANN(left, Y(99))));
	ASSERT_NO_THROW(other.append(SynapseOnHICANN(SynapseColumnOnHICANN(5), SynapseRowOnHICANN(0))));
	ASSERT_EQ(route, other);
}

TEST(L1Route, appendAfterBulk)
{
	{
		L1Route route{HICANNOnWafer(X(5), Y(5)), Merger0OnHICANN(2), DNCMergerOnHICANN(2),
		              HLineOnHICANN(46)};
		ASSERT_ANY_THROW(route.append(HICANNOnWafer(X(6), Y(5)), HLineOnHICANN(42)));
		ASSERT_NO_THROW(route.append(HICANNOnWafer(X(6), Y(5)), HLineOnHICANN(48)));
	}

	{
		L1Route route{HICANNOnWafer(X(5), Y(5)), Merger0OnHICANN(2),        DNCMergerOnHICANN(2),
		              HLineOnHICANN(46),         HICANNOnWafer(X(6), Y(5)), HLineOnHICANN(48)};
		ASSERT_ANY_THROW(route.append(HICANNOnWafer(X(7), Y(5)), HLineOnHICANN(46)));
		ASSERT_NO_THROW(route.append(HICANNOnWafer(X(7), Y(5)), HLineOnHICANN(50)));
	}
}

TEST(L1Route, extendEmptyWithEmpty)
{
	L1Route empty_route;
	empty_route.append(empty_route, L1Route::extend_mode::extend);
	ASSERT_TRUE(empty_route.empty());
	empty_route.prepend(empty_route, L1Route::extend_mode::extend);
	ASSERT_TRUE(empty_route.empty());
}

TEST(L1Route, extendWorksAcrossHICANNBoundaries)
{
	L1Route first{HICANNOnWafer(X(5), Y(5)), HLineOnHICANN(46)};
	L1Route second{HICANNOnWafer(X(6), Y(5)), HLineOnHICANN(48), VLineOnHICANN(39)};

	{
		L1Route copy = first;
		ASSERT_NO_THROW(copy.append(second, L1Route::extend_mode::extend));
	}

	{
		L1Route copy = second;
		ASSERT_NO_THROW(copy.prepend(first, L1Route::extend_mode::extend));
	}
}

TEST_F(ASimpleL1Route, extend)
{
	HICANNOnWafer hicann1(X(5), Y(5));
	HICANNOnWafer hicann2(X(6), Y(5));
	L1Route other{hicann1, Merger0OnHICANN(2)};
	ASSERT_ANY_THROW(
		other.append(L1Route{hicann1, HLineOnHICANN(42)}, L1Route::extend_mode::extend));
	ASSERT_ANY_THROW(
		other.append(L1Route{hicann2, HLineOnHICANN(48)}, L1Route::extend_mode::extend));
	ASSERT_NO_THROW(
		other.append(L1Route{hicann1, DNCMergerOnHICANN(2)}, L1Route::extend_mode::extend));
	ASSERT_NO_THROW(
		other.append(L1Route{hicann1, HLineOnHICANN(46)}, L1Route::extend_mode::extend));
	ASSERT_ANY_THROW(
		other.append(L1Route{hicann1, HLineOnHICANN(42)}, L1Route::extend_mode::extend));
	ASSERT_ANY_THROW(
		other.append(L1Route{hicann1, VLineOnHICANN(39)}, L1Route::extend_mode::extend));
	ASSERT_ANY_THROW(
		other.append(L1Route{hicann2, HLineOnHICANN(42)}, L1Route::extend_mode::extend));
	ASSERT_NO_THROW(
		other.append(L1Route{hicann2, HLineOnHICANN(48)}, L1Route::extend_mode::extend));
	ASSERT_NO_THROW(
		other.append(L1Route{hicann2, VLineOnHICANN(39)}, L1Route::extend_mode::extend));
	ASSERT_NO_THROW(
		other.append(
			L1Route{hicann2, SynapseDriverOnHICANN(left, Y(99))}, L1Route::extend_mode::extend));
	ASSERT_NO_THROW(
		other.append(
			L1Route{hicann2, SynapseOnHICANN(SynapseColumnOnHICANN(5), SynapseRowOnHICANN(0))},
			L1Route::extend_mode::extend));
	ASSERT_EQ(route, other);
}

TEST_F(ASimpleL1Route, extendPre)
{
	HICANNOnWafer hicann1(X(5), Y(5));
	HICANNOnWafer hicann2(X(6), Y(5));
	L1Route other{hicann2, SynapseOnHICANN(SynapseColumnOnHICANN(5), SynapseRowOnHICANN(0))};
	ASSERT_ANY_THROW(
		other.prepend(L1Route{hicann2, HLineOnHICANN(42)}, L1Route::extend_mode::extend));
	ASSERT_ANY_THROW(
		other.prepend(L1Route{hicann1, HLineOnHICANN(42)}, L1Route::extend_mode::extend));
	ASSERT_NO_THROW(
		other.prepend(
			L1Route{hicann2, SynapseDriverOnHICANN(left, Y(99))}, L1Route::extend_mode::extend));
	ASSERT_NO_THROW(
		other.prepend(L1Route{hicann2, VLineOnHICANN(39)}, L1Route::extend_mode::extend));
	ASSERT_NO_THROW(
		other.prepend(L1Route{hicann2, HLineOnHICANN(48)}, L1Route::extend_mode::extend));
	ASSERT_ANY_THROW(
		other.prepend(L1Route{hicann2, HLineOnHICANN(42)}, L1Route::extend_mode::extend));
	ASSERT_ANY_THROW(
		other.prepend(L1Route{hicann1, VLineOnHICANN(39)}, L1Route::extend_mode::extend));
	ASSERT_ANY_THROW(
		other.prepend(L1Route{hicann1, HLineOnHICANN(42)}, L1Route::extend_mode::extend));
	ASSERT_NO_THROW(
		other.prepend(L1Route{hicann1, HLineOnHICANN(46)}, L1Route::extend_mode::extend));
	ASSERT_NO_THROW(
		other.prepend(L1Route{hicann1, DNCMergerOnHICANN(2)}, L1Route::extend_mode::extend));
	ASSERT_NO_THROW(
		other.prepend(L1Route{hicann1, Merger0OnHICANN(2)}, L1Route::extend_mode::extend));
	ASSERT_EQ(route, other);
}

TEST_F(ASimpleL1Route, merge)
{
	HICANNOnWafer hicann1(X(5), Y(5));
	HICANNOnWafer hicann2(X(6), Y(5));
	L1Route first{hicann1, Merger0OnHICANN(2), DNCMergerOnHICANN(2), HLineOnHICANN(46)};
	L1Route second{hicann1,
	               HLineOnHICANN(46),
	               hicann2,
	               HLineOnHICANN(48),
	               VLineOnHICANN(39),
	               SynapseDriverOnHICANN(left, Y(99)),
	               SynapseOnHICANN(SynapseColumnOnHICANN(5), SynapseRowOnHICANN(0))};
	ASSERT_ANY_THROW(
	    first.append(
	        L1Route{hicann1, HLineOnHICANN(42)}, L1Route::extend_mode::merge_common_endpoints));
	ASSERT_ANY_THROW(
	    first.append(
	        L1Route{hicann2, HLineOnHICANN(46)}, L1Route::extend_mode::merge_common_endpoints));
	ASSERT_NO_THROW(first.append(second, L1Route::extend_mode::merge_common_endpoints));
	ASSERT_EQ(route, first);
}

TEST_F(ASimpleL1Route, mergePre)
{
	HICANNOnWafer hicann1(X(5), Y(5));
	HICANNOnWafer hicann2(X(6), Y(5));
	L1Route first{hicann1, Merger0OnHICANN(2), DNCMergerOnHICANN(2), HLineOnHICANN(46)};
	L1Route second{hicann1,
	               HLineOnHICANN(46),
	               hicann2,
	               HLineOnHICANN(48),
	               VLineOnHICANN(39),
	               SynapseDriverOnHICANN(left, Y(99)),
	               SynapseOnHICANN(SynapseColumnOnHICANN(5), SynapseRowOnHICANN(0))};
	L1Route route1{hicann1, HLineOnHICANN(42)};
	ASSERT_ANY_THROW(route1.prepend(first, L1Route::extend_mode::merge_common_endpoints));
	L1Route route2{hicann2, HLineOnHICANN(46)};
	ASSERT_ANY_THROW(route2.prepend(first, L1Route::extend_mode::merge_common_endpoints));
	ASSERT_NO_THROW(second.prepend(first, L1Route::extend_mode::merge_common_endpoints));
	ASSERT_EQ(route, second);
}

TEST(L1Route, findInvalid)
{
	L1Route::sequence_type route{};
	auto find_invalid = [](L1Route::sequence_type const& seq) {
		return L1Route::find_invalid(seq.begin(), seq.end());
	};

	ASSERT_EQ(route.end(), find_invalid(route));
	route.emplace_back(HICANNOnWafer(X(5), Y(5)));
	ASSERT_NE(route.end(), find_invalid(route));
	ASSERT_EQ(route.begin(), find_invalid(route));
	route.emplace_back(Merger0OnHICANN(2));
	ASSERT_EQ(route.end(), find_invalid(route));

	{
		auto route_copy = route;
		route_copy.emplace_back(HLineOnHICANN(42));
		ASSERT_NE(route_copy.end(), find_invalid(route_copy));
	}

	route.emplace_back(DNCMergerOnHICANN(2));
	route.emplace_back(HLineOnHICANN(46));
	ASSERT_EQ(route.end(), find_invalid(route));

	{
		auto route_copy = route;
		route_copy.emplace_back(HLineOnHICANN(42));
		ASSERT_NE(route_copy.end(), find_invalid(route_copy));
	}

	{
		auto route_copy = route;
		route_copy.emplace_back(VLineOnHICANN(39));
		ASSERT_NE(route_copy.end(), find_invalid(route_copy));
	}

	route.emplace_back(HICANNOnWafer(X(6), Y(5)));
	// HICANNOnWafer on its own is invalid.  Route has to end with non-HICANN segment.
	ASSERT_EQ(std::prev(route.end()), find_invalid(route));
	route.emplace_back(HLineOnHICANN(48));
	ASSERT_EQ(route.end(), find_invalid(route));
	route.emplace_back(VLineOnHICANN(39));
	route.emplace_back(SynapseDriverOnHICANN(left, Y(99)));
	route.emplace_back(SynapseOnHICANN(SynapseColumnOnHICANN(5), SynapseRowOnHICANN(0)));
	ASSERT_EQ(route.end(), find_invalid(route));
}

TEST_F(ASimpleL1Route, canNotEndWithHICANN)
{
	EXPECT_ANY_THROW(L1Route route{HICANNOnWafer(X(5), Y(5))});
	try {
	    L1Route route{HICANNOnWafer(X(5), Y(5)), HLineOnHICANN(46), HICANNOnWafer(X(6), Y(5))};
	    FAIL() << "route that ends with HICANN did not throw";
	} catch (InvalidRouteError const&) {
	}
}

TEST_F(ASimpleL1Route, isNotEmpty)
{
	ASSERT_FALSE(route.empty());
}

TEST_F(ASimpleL1Route, hasSize)
{
	ASSERT_EQ(9, route.size());
}

TEST_F(ASimpleL1Route, hasSourceAndTargetHICANN)
{
	ASSERT_EQ(HICANNOnWafer(X(5), Y(5)), route.source_hicann());
	ASSERT_EQ(HICANNOnWafer(X(6), Y(5)), route.target_hicann());
}

TEST_F(ASimpleL1Route, canBeCompared)
{
	ASSERT_TRUE(route != L1Route());
	ASSERT_FALSE(route == L1Route());
}

TEST_F(ASimpleL1Route, doesNotChangeWhenExtendedWithEmptyRoute)
{
	L1Route empty_route;
	L1Route original = route;
	route.append(empty_route, L1Route::extend_mode::extend);
	ASSERT_EQ(original, route);
}

TEST_F(ASimpleL1Route, doesNotChangeWhenMergedWithEmptyRoute)
{
	L1Route empty_route;
	L1Route original = route;
	route.append(empty_route, L1Route::extend_mode::merge_common_endpoints);
	ASSERT_EQ(original, route);
}

TEST_F(ASimpleL1Route, canBeUsedToExtendAnEmptyRoute)
{
	L1Route empty_route;
	empty_route.append(route, L1Route::extend_mode::extend);
	ASSERT_EQ(route, empty_route);
}

TEST_F(ASimpleL1Route, canBeMergedToAnEmptyRoute)
{
	L1Route empty_route;
	empty_route.append(route, L1Route::extend_mode::merge_common_endpoints);
	ASSERT_EQ(route, empty_route);
}

TEST_F(ASimpleL1Route, canBeSplit)
{
	{
		auto res = route.split(route.begin());
		EXPECT_TRUE(res.first.empty());
		EXPECT_EQ(route, res.second);
		EXPECT_EQ(route.source_hicann(), res.second.source_hicann());
		EXPECT_EQ(route.target_hicann(), res.second.target_hicann());
	}

	{
		auto res = route.split(std::next(route.begin()));
		EXPECT_TRUE(res.first.empty());
		EXPECT_EQ(route, res.second);
		EXPECT_EQ(route.source_hicann(), res.second.source_hicann());
		EXPECT_EQ(route.target_hicann(), res.second.target_hicann());
	}

	HICANNOnWafer hicann1(X(5), Y(5));
	HICANNOnWafer hicann2(X(6), Y(5));

	{
		auto res = route.split(std::next(route.begin(), 2));
		EXPECT_FALSE(res.first.empty());
		EXPECT_FALSE(res.second.empty());
		L1Route first{hicann1, Merger0OnHICANN(2)};
		EXPECT_EQ(first, res.first);
		EXPECT_EQ(hicann1, res.first.source_hicann());
		EXPECT_EQ(hicann1, res.first.target_hicann());
		EXPECT_EQ(8, res.second.size());
		EXPECT_EQ(hicann1, res.second.source_hicann());
		EXPECT_EQ(hicann2, res.second.target_hicann());
		EXPECT_EQ(L1Route::segment_type(DNCMergerOnHICANN(2)), res.second.front());
	}

	{
		auto res = route.split(std::next(route.begin(), 4));
		auto res2 = route.split(std::next(route.begin(), 5));
		EXPECT_EQ(res.first, res2.first);
		EXPECT_EQ(res.second, res2.second);
		L1Route first{hicann1, Merger0OnHICANN(2), DNCMergerOnHICANN(2), HLineOnHICANN(46)};
		L1Route second{hicann2, HLineOnHICANN(48), VLineOnHICANN(39),
		               SynapseDriverOnHICANN(left, Y(99)),
		               SynapseOnHICANN(SynapseColumnOnHICANN(5), SynapseRowOnHICANN(0))};
		EXPECT_EQ(first, res.first);
		EXPECT_EQ(hicann1, res.first.source_hicann());
		EXPECT_EQ(hicann1, res.first.target_hicann());
		EXPECT_EQ(second, res.second);
		EXPECT_EQ(hicann2, res.second.source_hicann());
		EXPECT_EQ(hicann2, res.second.target_hicann());
	}

	{
		auto res = route.split(route.end());
		EXPECT_EQ(route, res.first);
		EXPECT_TRUE(res.second.empty());
		EXPECT_EQ(route.source_hicann(), res.first.source_hicann());
		EXPECT_EQ(route.target_hicann(), res.first.target_hicann());
	}
}

TEST(L1Route, outputToLeftOfSendingRepeater)
{
	L1Route route{HICANNOnWafer(X(5), Y(5)), DNCMergerOnHICANN(2)};
	ASSERT_ANY_THROW(route.append(HICANNOnWafer(X(4), Y(5)), HLineOnHICANN(46)));
	ASSERT_NO_THROW(route.append(HICANNOnWafer(X(4), Y(5)), HLineOnHICANN(44)));
}

TEST(L1Route, insertIntoAdjacentSynapseArray)
{
	L1Route route{HICANNOnWafer(X(5), Y(5)), VLineOnHICANN(2)};
	ASSERT_ANY_THROW(route.append(HICANNOnWafer(X(4), Y(5)), SynapseDriverOnHICANN(left, Y(99))));
	ASSERT_ANY_THROW(route.append(HICANNOnWafer(X(5), Y(5)), SynapseDriverOnHICANN(right, Y(100))));
	ASSERT_ANY_THROW(route.append(HICANNOnWafer(X(6), Y(5)), SynapseDriverOnHICANN(left, Y(99))));
	ASSERT_ANY_THROW(route.append(HICANNOnWafer(X(6), Y(5)), SynapseDriverOnHICANN(right, Y(100))));
	ASSERT_NO_THROW(route.append(HICANNOnWafer(X(4), Y(5)), SynapseDriverOnHICANN(right, Y(100))));
}

TEST(L1Route, connectedSynapseDriversUp)
{
	L1Route route{HICANNOnWafer(Enum(1)), VLineOnHICANN(4)};
	ASSERT_NO_THROW(route.append(HICANNOnWafer(Enum(1)), SynapseDriverOnHICANN(Enum(3))));
	ASSERT_NO_THROW(route.append(HICANNOnWafer(Enum(1)), SynapseDriverOnHICANN(Enum(1))));
}

TEST(L1Route, connectedSynapseDriversDown)
{
	L1Route route{HICANNOnWafer(Enum(1)), VLineOnHICANN(4)};
	ASSERT_NO_THROW(route.append(HICANNOnWafer(Enum(1)), SynapseDriverOnHICANN(Enum(3))));
	ASSERT_NO_THROW(route.append(HICANNOnWafer(Enum(1)), SynapseDriverOnHICANN(Enum(5))));
}

TEST(L1Route, connectedSynapseDriversCenter)
{
	L1Route route{HICANNOnWafer(Enum(1)), VLineOnHICANN(4)};
	ASSERT_NO_THROW(route.append(HICANNOnWafer(Enum(1)), SynapseDriverOnHICANN(Enum(3))));
	ASSERT_NO_THROW(route.append(HICANNOnWafer(Enum(1)), SynapseDriverOnHICANN(Enum(5))));
	// in order to append Driver 1 we need to go back to the centre
	ASSERT_NO_THROW(route.append(HICANNOnWafer(Enum(1)), SynapseDriverOnHICANN(Enum(3))));
	ASSERT_NO_THROW(route.append(HICANNOnWafer(Enum(1)), SynapseDriverOnHICANN(Enum(1))));
}

} // namespace marocco
