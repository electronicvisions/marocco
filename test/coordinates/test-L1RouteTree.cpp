#include "test/common.h"

#include "marocco/coordinates/L1RouteTree.h"

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

TEST(L1RouteTree, canBeCopied)
{
	L1Route route{HICANNOnWafer(X(5), Y(5)), HLineOnHICANN(46)};
	L1RouteTree tree(route);
	L1RouteTree copy(tree);
	ASSERT_EQ(tree, copy);
}

TEST(L1RouteTree, hasHead)
{
	L1Route route{HICANNOnWafer(X(5), Y(5)), HLineOnHICANN(46)};
	L1RouteTree tree(route);
	ASSERT_EQ(route, tree.head());
}

TEST(L1RouteTree, canHaveTails)
{
	L1Route route{HICANNOnWafer(X(5), Y(5)), HLineOnHICANN(48)};
	L1RouteTree tree(route);
	EXPECT_FALSE(tree.has_tails());
	tree.add(L1Route{HICANNOnWafer(X(5), Y(5)), HLineOnHICANN(48), VLineOnHICANN(39)});
	ASSERT_TRUE(tree.has_tails());
}

TEST(L1RouteTree, hasEmptyTailWhenTargetIsReachedEnPassant)
{
	L1Route route{HICANNOnWafer(X(5), Y(5)), Merger0OnHICANN(2), DNCMergerOnHICANN(2),
	              HLineOnHICANN(46)};
	L1Route route_reached_en_passant(L1Route::sequence_type(route.begin(), std::prev(route.end())));

	{
		L1RouteTree tree(route);
		tree.add(route_reached_en_passant);
		EXPECT_EQ(route_reached_en_passant, tree.head());
		EXPECT_TRUE(tree.has_tails());
		auto tails = tree.tails();
		ASSERT_EQ(2, tails.size());
		EXPECT_FALSE(tails[0].get().empty());
		EXPECT_EQ(route.back(), tails[0].get().head().back());
		EXPECT_TRUE(tails[1].get().empty());
	}

	{
		L1RouteTree tree(route_reached_en_passant);
		tree.add(route);
		EXPECT_EQ(route_reached_en_passant, tree.head());
		EXPECT_TRUE(tree.has_tails());
		auto tails = tree.tails();
		ASSERT_EQ(2, tails.size());
		EXPECT_FALSE(tails[0].get().empty());
		EXPECT_EQ(route.back(), tails[0].get().head().back());
		EXPECT_TRUE(tails[1].get().empty());
	}
}

TEST(L1RouteTree, canBeEmpty)
{
	L1Route route{HICANNOnWafer(X(5), Y(5)), HLineOnHICANN(46)};
	L1RouteTree tree(route);
	EXPECT_FALSE(tree.empty());
	L1RouteTree empty_tree{};
	EXPECT_TRUE(empty_tree.empty());
}

TEST(L1RouteTree, doesNotAcceptDisjunctRoute)
{
	L1Route route{HICANNOnWafer(X(5), Y(5)), HLineOnHICANN(46)};
	L1RouteTree tree(route);
	ASSERT_ANY_THROW(tree.add(L1Route{HICANNOnWafer(X(5), Y(5)), HLineOnHICANN(2)}));
	ASSERT_ANY_THROW(tree.add(L1Route{HICANNOnWafer(X(6), Y(5)), HLineOnHICANN(46)}));

	L1RouteTree empty_tree{};
	ASSERT_NO_THROW(empty_tree.add(L1Route{HICANNOnWafer(X(5), Y(5)), HLineOnHICANN(2)}));
}

TEST(L1RouteTree, doesNotCompareEqualWhenTailsIsSubset)
{
	L1Route head{HICANNOnWafer(X(5), Y(5)), HLineOnHICANN(48), VLineOnHICANN(7)};
	L1RouteTree less_tails(head);
	less_tails.add(L1Route{HICANNOnWafer(X(5), Y(5)), HLineOnHICANN(48), VLineOnHICANN(39)});
	L1RouteTree more_tails = less_tails;
	more_tails.add(L1Route{HICANNOnWafer(X(5), Y(5)), HLineOnHICANN(48), VLineOnHICANN(71)});
	EXPECT_NE(less_tails, more_tails);
	EXPECT_NE(more_tails, less_tails);
}

} // namespace marocco
