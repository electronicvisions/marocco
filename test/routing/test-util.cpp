#include "test/common.h"

#include <bitset>
#include <boost/dynamic_bitset.hpp>

#include "marocco/routing/util.h"

namespace marocco {
namespace routing {

TEST(Routing, relative_index)
{
	boost::dynamic_bitset<> mask(8);
	ASSERT_TRUE(mask.none());
	EXPECT_ANY_THROW(to_relative_index(mask, 2));
	mask.flip(2);
	EXPECT_EQ(0, to_relative_index(mask, 2));
	EXPECT_EQ(2, from_relative_index(mask, to_relative_index(mask, 2)));
	mask.flip(1);
	EXPECT_EQ(1, to_relative_index(mask, 2));
	EXPECT_EQ(2, from_relative_index(mask, to_relative_index(mask, 2)));
	mask.flip(0);
	EXPECT_EQ(2, to_relative_index(mask, 2));
	EXPECT_EQ(2, from_relative_index(mask, to_relative_index(mask, 2)));
	mask.flip(5);
	EXPECT_ANY_THROW(to_relative_index(mask, 6));
	mask.flip(6);
	EXPECT_EQ(4, to_relative_index(mask, 6));
	EXPECT_EQ(6, from_relative_index(mask, to_relative_index(mask, 6)));
	EXPECT_ANY_THROW(to_relative_index(mask, 10));

	// different mask length
	boost::dynamic_bitset<> mask_2(10);
	mask_2.flip(3);
	EXPECT_EQ(0, to_relative_index(mask_2,3));
}

} // routing
} // marocco
