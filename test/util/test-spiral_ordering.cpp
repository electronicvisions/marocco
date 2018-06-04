#include "test/common.h"

#include "marocco/util/spiral_ordering.h"

#include <algorithm>
#include <vector>
#include <set>

#include "hal/Coordinate/HMFGeometry.h"
#include "hal/Coordinate/iter_all.h"
#include "hal/Coordinate/typed_array.h"

using namespace HMF::Coordinate;

namespace marocco {

struct SpiralOrderingOfHICANNs : public ::testing::Test
{
	SpiralOrderingOfHICANNs()
	{
		hicanns.reserve(HICANNOnWafer::enum_type::size);

		for (auto hicann : iter_all<HICANNOnWafer>()) {
			hicanns.push_back(hicann);
		}

		std::sort(hicanns.begin(), hicanns.end(), ordering);
	}

	std::vector<HICANNOnWafer> hicanns;
	spiral_ordering<HICANNOnWafer> ordering;
}; // SpiralOrderingOfHICANNs

TEST_F(SpiralOrderingOfHICANNs, MatchesOldImplementation)
{
	std::vector<size_t> ids = {
	    174, 210, 209, 173, 137, 138, 139, 175, 211, 247, 246, 245, 244, 208, 172, 136, 104, 105,
	    106, 107, 108, 140, 176, 212, 248, 280, 279, 278, 277, 276, 275, 243, 207, 171, 135, 103,
	    75,  76,  77,  78,  79,  80,  81,  109, 141, 177, 213, 249, 281, 309, 308, 307, 306, 305,
	    304, 303, 302, 274, 242, 206, 170, 134, 102, 74,  50,  51,  52,  53,  54,  55,  56,  57,
	    58,  82,  110, 142, 178, 214, 250, 282, 310, 334, 333, 332, 331, 330, 329, 328, 327, 326,
	    325, 301, 273, 241, 205, 169, 133, 101, 73,  49,  29,  30,  31,  32,  33,  34,  35,  36,
	    37,  38,  39,  59,  83,  111, 143, 179, 215, 251, 283, 311, 335, 355, 354, 353, 352, 351,
	    350, 349, 348, 347, 346, 345, 344, 324, 300, 272, 240, 204, 168, 132, 100, 72,  48,  28,
	    12,  13,  14,  15,  16,  17,  18,  19,  20,  21,  22,  23,  40,  60,  84,  112, 144, 180,
	    216, 252, 284, 312, 336, 356, 371, 370, 369, 368, 367, 366, 365, 364, 363, 362, 361, 360,
	    343, 323, 299, 271, 239, 203, 167, 131, 99,  71,  47,  27,  0,   1,   2,   3,   4,   5,
	    6,   7,   8,   9,   10,  11,  41,  61,  85,  113, 145, 181, 217, 253, 285, 313, 337, 357,
	    383, 382, 381, 380, 379, 378, 377, 376, 375, 374, 373, 372, 342, 322, 298, 270, 238, 202,
	    166, 130, 98,  70,  46,  26,  42,  62,  86,  114, 146, 182, 218, 254, 286, 314, 338, 358,
	    341, 321, 297, 269, 237, 201, 165, 129, 97,  69,  45,  25,  43,  63,  87,  115, 147, 183,
	    219, 255, 287, 315, 339, 359, 340, 320, 296, 268, 236, 200, 164, 128, 96,  68,  44,  24,
	    88,  116, 148, 184, 220, 256, 288, 316, 295, 267, 235, 199, 163, 127, 95,  67,  89,  117,
	    149, 185, 221, 257, 289, 317, 294, 266, 234, 198, 162, 126, 94,  66,  90,  118, 150, 186,
	    222, 258, 290, 318, 293, 265, 233, 197, 161, 125, 93,  65,  91,  119, 151, 187, 223, 259,
	    291, 319, 292, 264, 232, 196, 160, 124, 92,  64,  152, 188, 224, 260, 231, 195, 159, 123,
	    153, 189, 225, 261, 230, 194, 158, 122, 154, 190, 226, 262, 229, 193, 157, 121, 155, 191,
	    227, 263, 228, 192, 156, 120};

	size_t ii = 0;
	for (auto const& hicann : hicanns) {
		EXPECT_EQ(HICANNOnWafer(Enum(ids[ii])), hicann) << "at " << ii;
		++ii;
	}
}

TEST_F(SpiralOrderingOfHICANNs, HasMonotonicallyIncreasingDistance)
{
	double last_distance = 0.0;
	for (auto const& hicann : hicanns) {
		double const distance = ordering.distance(hicann);
		ASSERT_LE(last_distance, distance);
		last_distance = distance;
	}
}

TEST_F(SpiralOrderingOfHICANNs, HasNoCollisionsWhenUsedInSet)
{
	std::set<HICANNOnWafer, spiral_ordering<HICANNOnWafer> > set;

	for (auto hicann : iter_all<HICANNOnWafer>()) {
		set.insert(hicann);
	}

	ASSERT_EQ(HICANNOnWafer::enum_type::size, set.size());
}

TEST_F(SpiralOrderingOfHICANNs, PreservesTopologicalAdjacency)
{
	std::array<std::array<int, HICANNOnWafer::y_type::size>, HICANNOnWafer::x_type::size> grid;

	std::fill(grid.begin()->begin(), grid.rbegin()->end(), -1);
	{
		int idx = 0;
	    for (auto const& hicann : hicanns) {
		    grid.at(hicann.x()).at(hicann.y()) = idx++;
	    }
    }

    // for (size_t yy = 0; yy < HICANNOnWafer::y_type::end; ++yy) {
	// 	for (size_t xx = 0; xx < HICANNOnWafer::x_type::end; ++xx) {
	// 		std::cout << std::setw(4) << grid.at(xx).at(yy);
	// 	}
	// 	std::cout << std::endl;
	// }

	for (auto const& hicann : hicanns) {
		auto xx = hicann.x();
		auto yy = hicann.y();

		int const val = grid.at(xx).at(yy);

		// avoid corner cases
		if ((xx == 0 || yy == 0) ||
		    (xx == HICANNOnWafer::x_type::max || yy == HICANNOnWafer::y_type::max))
			continue;

		std::vector<int> adj{
			grid.at(xx).at(yy - 1),
			grid.at(xx).at(yy + 1),
			grid.at(xx - 1).at(yy),
			grid.at(xx + 1).at(yy)
		};

		// avoid more corner cases
		if (std::find(adj.begin(), adj.end(), -1) != adj.end())
			continue;

		// check that any of the adjacent elements is sucessor
		if (val < int(HICANNOnWafer::enum_type::max)) {
			EXPECT_NE(adj.end(), std::find(adj.begin(), adj.end(), val + 1)) << val;
		}

		// check that any of the adjacent elements is predecessor
		if (val > 0) {
			EXPECT_NE(adj.end(), std::find(adj.begin(), adj.end(), val - 1)) << val;
		}
	}
}

struct SpiralOrderingOfHICANNsShiftedCenter : public ::testing::Test
{
	SpiralOrderingOfHICANNsShiftedCenter()
	    : ordering(HICANNOnWafer(Enum(304)).x(), HICANNOnWafer(Enum(304)).y())
	{
		hicanns.reserve(HICANNOnWafer::enum_type::size);

		for (auto hicann : iter_all<HICANNOnWafer>()) {
			hicanns.push_back(hicann);
		}

		std::sort(hicanns.begin(), hicanns.end(), ordering);
	}

	std::vector<HICANNOnWafer> hicanns;
	spiral_ordering<HICANNOnWafer> ordering;
}; // SpiralOrderingOfHICANNsShiftedCenter

TEST_F(SpiralOrderingOfHICANNsShiftedCenter, ShiftingCenterWorks)
{
	std::vector<size_t> ids = {304, 276, 277, 305, 329, 328, 327, 303, 275, 243};

	size_t ii = 0;
	for (auto const& id : ids) {
		EXPECT_EQ(HICANNOnWafer(Enum(id)), hicanns[ii]) << "at " << ii;
		++ii;
	}
}

} // namespace marocco
