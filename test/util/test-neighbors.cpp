#include <algorithm>
#include <array>
#include <set>

#include "marocco/util/neighbors.h"
#include "test/common.h"

#include "halco/common/iter_all.h"
#include "halco/hicann/v2/hicann.h"

using namespace halco::hicann::v2;
using namespace halco::common;

namespace marocco {

TEST(Neighbors, CanUseCopyConstructorOfContainers)
{
	std::array<HICANNOnWafer, 2> hicanns{{HICANNOnWafer(Enum(2)), HICANNOnWafer(Enum(3))}};
	Neighbors<HICANNOnWafer> neighbors(hicanns.begin(), hicanns.end());
	ASSERT_EQ(2, neighbors.points().size());
	ASSERT_TRUE(std::equal(hicanns.begin(), hicanns.end(), neighbors.points().begin()));
}

class NeighborsWithHICANNs : public ::testing::Test
{
public:
	NeighborsWithHICANNs() : neighbors()
	{
	}

	void add_all_hicanns()
	{
		neighbors.reserve(HICANNOnWafer::enum_type::size);
		for (auto hicann : iter_all<HICANNOnWafer>()) {
			neighbors.push_back(hicann);
		}
	}

	static std::set<HICANNOnWafer> closest_hicanns_to(HICANNOnWafer const& hicann)
	{
		std::set<HICANNOnWafer> closest;
		for (int x_offset = -1; x_offset <= 1; ++x_offset) {
			for (int y_offset = -1; y_offset <= 1; ++y_offset) {
				closest.insert(HICANNOnWafer(X(hicann.x() + x_offset), Y(hicann.y() + y_offset)));
			}
		}
		return closest;
	}

	void expect_single_result(HICANNOnWafer const& hicann) const
	{
		EXPECT_EQ(1, neighbors.indices().size());
		EXPECT_EQ(1, neighbors.squared_distances().size());
		EXPECT_NE(neighbors.begin(), neighbors.end());
		ASSERT_EQ(hicann, *neighbors.begin());
	}

	Neighbors<HICANNOnWafer> neighbors;
};

TEST_F(NeighborsWithHICANNs, CanReserveSpace)
{
	neighbors.reserve(5);
}

TEST_F(NeighborsWithHICANNs, CanBePushedTo)
{
	ASSERT_EQ(0, neighbors.points().size());
	neighbors.push_back(HICANNOnWafer(Enum(2)));
	ASSERT_EQ(1, neighbors.points().size());
}

TEST_F(NeighborsWithHICANNs, ChecksNumberOfPoints)
{
	HICANNOnWafer hicann{X(12), Y(13)};
	ASSERT_ANY_THROW(neighbors.find_near(hicann, 1));
	neighbors.push_back(hicann);
	ASSERT_NO_THROW(neighbors.find_near(hicann, 1));
}

TEST_F(NeighborsWithHICANNs, CanFindSingleCoordinate)
{
	HICANNOnWafer hicann{X(12), Y(13)};
	ASSERT_ANY_THROW(neighbors.find_near(hicann, 1));

	neighbors.push_back(hicann);

	neighbors.find_near(hicann, 1);
	// Should be a perfect match for itself.
	expect_single_result(hicann);
}

TEST_F(NeighborsWithHICANNs, CanFindNeighborsOfCoordinate)
{
	add_all_hicanns();
	HICANNOnWafer hicann{X(12), Y(10)};

	neighbors.find_near(hicann, 1);
	// Should be a perfect match for itself.
	expect_single_result(hicann);

	neighbors.find_near(hicann, 9);
	EXPECT_EQ(9, neighbors.indices().size());
	EXPECT_EQ(9, neighbors.squared_distances().size());
	auto closest = closest_hicanns_to(hicann);
	std::set<HICANNOnWafer> found(neighbors.begin(), neighbors.end());
	ASSERT_EQ(closest, found);
}

TEST_F(NeighborsWithHICANNs, CanFindNeighborsOfPoint)
{
	add_all_hicanns();

	HICANNOnWafer hicann{X(12), Y(10)};
	float xx = float(hicann.x()) + 0.1;
	float yy = float(hicann.y()) + 0.1;

	neighbors.find_near(xx, yy, 1);
	expect_single_result(hicann);

	neighbors.find_near(xx, yy, 9);
	EXPECT_EQ(9, neighbors.indices().size());
	EXPECT_EQ(9, neighbors.squared_distances().size());
	auto closest = closest_hicanns_to(hicann);
	std::set<HICANNOnWafer> found(neighbors.begin(), neighbors.end());
	ASSERT_EQ(closest, found);
}

TEST_F(NeighborsWithHICANNs, ProvidesDistance)
{
	add_all_hicanns();
	HICANNOnWafer hicann{X(12), Y(10)};

	neighbors.find_near(hicann, 1);
	auto it = neighbors.begin();
	ASSERT_FLOAT_EQ(0.0, neighbors.distance(it));

	neighbors.find_near(hicann, 9);
	for (auto it = neighbors.begin(), eit = neighbors.end(); it != eit; ++it) {
		EXPECT_LE(neighbors.distance(it), std::sqrt(2.0)) << "for " << *it;
	}
}

} // namespace marocco
