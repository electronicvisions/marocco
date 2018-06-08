#include <array>
#include <functional>
#include <iterator>

#include "marocco/alg.h"
#include "test/common.h"

namespace marocco {

TEST(AlgCountWhile, WorksOnArray) {
	/*                           0  1  2  3  4  5  6  7  8  9 10 11 */
	std::array<bool, 12> assign{{0, 0, 1, 0, 0, 1, 1, 0, 1, 0, 0, 0}};

	auto unassigned_p = std::logical_not<bool>();

	// forwards

	EXPECT_EQ(2, alg::count_while(assign.begin(), assign.end(), unassigned_p));

	auto pos = assign.begin() + 1;
	EXPECT_EQ(1, alg::count_while(pos, assign.end(), unassigned_p));

	pos = assign.begin() + 2;
	EXPECT_EQ(0, alg::count_while(pos, assign.end(), unassigned_p));

	pos = assign.begin() + 3;
	EXPECT_EQ(2, alg::count_while(pos, assign.end(), unassigned_p));

	pos = assign.begin() + 9;
	EXPECT_EQ(3, alg::count_while(pos, assign.end(), unassigned_p));

	// backwards

	using reverse = decltype(assign)::reverse_iterator;

	pos = assign.begin() + 1;
	auto rpos = reverse(pos + 1);
	EXPECT_EQ(2, alg::count_while(rpos, assign.rend(), unassigned_p));

	pos = assign.begin() + 2;
	rpos = reverse(pos + 1);
	EXPECT_EQ(0, alg::count_while(rpos, assign.rend(), unassigned_p));

	pos = assign.begin() + 3;
	rpos = reverse(pos + 1);
	EXPECT_EQ(1, alg::count_while(rpos, assign.rend(), unassigned_p));

	pos = assign.begin() + 9;
	rpos = reverse(pos + 1);
	EXPECT_EQ(1, alg::count_while(rpos, assign.rend(), unassigned_p));

	EXPECT_EQ(3, alg::count_while(assign.rbegin(), assign.rend(), unassigned_p));

	// "gap" calculation

	pos = assign.begin() + 3;
	rpos = reverse(pos);
	EXPECT_EQ(2,
	          // look below
	          alg::count_while(pos, assign.end(), unassigned_p) +
	          // look above
	          alg::count_while(rpos, assign.rend(), unassigned_p));

	pos = assign.begin() + 10;
	rpos = reverse(pos);
	EXPECT_EQ(3,
	          // look below
	          alg::count_while(pos, assign.end(), unassigned_p) +
	          // look above
	          alg::count_while(rpos, assign.rend(), unassigned_p));
}

TEST(AlgArithmethicMean, WorksOnSimpleExamples) {
	{
		std::vector<double> const v{2.0, 2.0, 2.0};
		EXPECT_DOUBLE_EQ(2.0, alg::arithmetic_mean(v.begin(), v.end()));
	}

	{
		std::vector<double> const v{1.0, 2.0, 3.0};
		EXPECT_DOUBLE_EQ(2.0, alg::arithmetic_mean(v.begin(), v.end()));
	}

	{
		std::vector<double> const v{1.0, 1.5, 2.0};
		EXPECT_DOUBLE_EQ(1.5, alg::arithmetic_mean(v.begin(), v.end()));
	}
}

} // namespace marocco
