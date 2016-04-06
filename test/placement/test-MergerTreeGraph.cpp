#include "test/common.h"

#include <boost/preprocessor/cat.hpp>

#include "hal/Coordinate/iter_all.h"
#include "marocco/placement/MergerTreeGraph.h"

using namespace HMF::Coordinate;

namespace marocco {
namespace placement {

class AMergerTreeGraph : public ::testing::Test
{
public:
	AMergerTreeGraph() : mtg(), graph(mtg.graph())
	{
	}

	MergerTreeGraph mtg;
	MergerTreeGraph::graph_type const& graph;
}; // AMergerTreeGraph

TEST_F(AMergerTreeGraph, usesCorrectVertexDescriptors)
{
	for (auto merger : iter_all<DNCMergerOnHICANN>()) {
		auto vertex = mtg[merger];
		EXPECT_EQ(4, graph[vertex].level);
		EXPECT_EQ(merger.value(), graph[vertex].index);
	}

#define CHECK_VERTICES(LEVEL)                                                                      \
	for (auto merger : iter_all<BOOST_PP_CAT(BOOST_PP_CAT(Merger, LEVEL), OnHICANN)>()) {          \
		auto vertex = mtg[merger];                                                                 \
		EXPECT_EQ(LEVEL, graph[vertex].level);                                                     \
		EXPECT_EQ(merger.value(), graph[vertex].index); \
	}

	CHECK_VERTICES(3);
	CHECK_VERTICES(2);
	CHECK_VERTICES(1);
	CHECK_VERTICES(0);
#undef CHECK_VERTICES
}

TEST_F(AMergerTreeGraph, hasTheRightDimensions)
{
	EXPECT_EQ(15 + 8, mtg.vertices_count());
	EXPECT_EQ(mtg.vertices_count(), num_vertices(graph));
	EXPECT_EQ(2 * 7 + 8, num_edges(graph));
}

TEST_F(AMergerTreeGraph, canBeCopied)
{
	MergerTreeGraph copy = mtg;
	copy.remove(DNCMergerOnHICANN(5));
	EXPECT_EQ(2 * 7 + 8, num_edges(graph));
	EXPECT_EQ(2 * 7 + 7, num_edges(copy.graph()));
}

} // placement
} // marocco
