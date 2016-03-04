#include <functional>

#include "test/common.h"
#include "marocco/util/to_function.h"

namespace marocco {

TEST(ToFunction, WorksForNonCapturingLambda)
{
	auto f = to_function([](int x) -> int { return x * 2; });
	ASSERT_EQ(8, f(4));
}

TEST(ToFunction, WorksForCapturingLambda)
{
	int q = 5;
	auto f = to_function([q]() mutable -> int { return ++q; });
	ASSERT_EQ(6, f());
	ASSERT_EQ(7, f());
	EXPECT_EQ(5, q);
}

class ClassWithMemberFuns
{
public:
	bool a(int x) const
	{
		return x == 42;
	}

	bool b(int x)
	{
		if (x == val) {
			return false;
		}
		val = x;
		return true;
	}

private:
	int val = 5;
}; // ClassWithMemberFuns

TEST(ToFunction, WorksForBindingConstMemFun)
{
	ClassWithMemberFuns cl;

	using namespace std::placeholders;
	auto f1 = to_function(&ClassWithMemberFuns::a, cl, _1);
	ASSERT_TRUE(f1(42));
	auto f2 = to_function(&ClassWithMemberFuns::a, &cl, _1);
	ASSERT_TRUE(f2(42));
}

TEST(ToFunction, WorksForBindingMemFun)
{
	ClassWithMemberFuns cl;

	using namespace std::placeholders;
	auto f1 = to_function(&ClassWithMemberFuns::b, cl, _1);
	ASSERT_TRUE(f1(42));
	auto f2 = to_function(&ClassWithMemberFuns::b, &cl, _1);
	ASSERT_FALSE(f2(2));
}

} // namespace marocco
