#include <algorithm>
#include <vector>
#include <iterator>
#include <type_traits>

#include "marocco/util/enum_iterator.h"
#include "test/common.h"
#include "halco/common/typed_array.h"

namespace marocco {

enum class ABC
{
	A,
	B,
	C
};

using std::is_same;
typedef enum_iterator<ABC, ABC::A, ABC::B, ABC::C> it_type;
static_assert(
	is_same<it_type::value_type, ABC const>::value, "enum_iterator has wrong value_type.");
static_assert(
	is_same<it_type::difference_type, std::ptrdiff_t>::value,
	"enum_iterator has wrong difference_type.");
static_assert(
	is_same<it_type::pointer, ABC const*>::value, "enum_iterator has wrong pointer type.");
static_assert(
	is_same<it_type::reference, ABC const&>::value, "enum_iterator has wrong reference type.");
static_assert(it_type::begin == 0, "enum_iterator has wrong begin value.");
static_assert(it_type::end == 3, "enum_iterator has wrong end value.");
static_assert(it_type::max == 2, "enum_iterator has wrong max value.");
static_assert(it_type::min == 0, "enum_iterator has wrong min value.");
static_assert(it_type::size == 3, "enum_iterator has wrong size.");

TEST(EnumIterator, MaintainsGivenOrder)
{
	std::vector<ABC> values;
	std::copy(it_type(0), it_type(), std::back_inserter(values));
	EXPECT_EQ(3, values.size());
	EXPECT_EQ(ABC::A, values[0]);
	EXPECT_EQ(ABC::B, values[1]);
	EXPECT_EQ(ABC::C, values[2]);
}

TEST(EnumIterator, MayStartAtArbitraryPosition)
{
	std::vector<ABC> values;
	std::copy(it_type(ABC::B), it_type(), std::back_inserter(values));
	EXPECT_EQ(2, values.size());
	EXPECT_EQ(ABC::B, values[0]);
	EXPECT_EQ(ABC::C, values[1]);
}

TEST(EnumIterator, CanProvideLimitsForTypedArray)
{
	halco::common::typed_array<bool, ABC, it_type> test;
	test.fill(false);
	EXPECT_EQ(3, test.size());
	test[ABC::A] = true;
	EXPECT_TRUE(test.at(ABC::A));
}

} // namespace marocco
