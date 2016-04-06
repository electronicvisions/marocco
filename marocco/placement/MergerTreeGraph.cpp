#include "marocco/placement/MergerTreeGraph.h"

#include <boost/preprocessor/cat.hpp>

#include "hal/Coordinate/iter_all.h"

using namespace HMF::Coordinate;

namespace marocco {
namespace placement {

MergerTreeGraph::MergerTreeGraph() : m_graph()
{
	// Lookup of vertex descriptors via `operator[]()` without an additional table depends
	// on the order of `add_vertex()` calls.  Previously vertices were added implicitly by
	// accessing their bundled properties (i.e. m_graph[ii] = X; where ii is the desired
	// vertex descriptor), but this does not seem to be documented officially.
	// The current method is sanctioned by
	// ,---- [http://www.boost.org/doc/libs/1_60_0/libs/graph/doc/adjacency_list.html]
	// | “If the VertexList of the graph is vecS, then the graph has a builtin vertex
	// | indices accessed via the property map for the vertex_index_t property. The
	// | indices fall in the range [0, num_vertices(g)) and are contiguous. When a vertex
	// | is removed the indices are adjusted so that they retain these properties.”
	// `----
	// Note that in order to keep this static assignment, we cannot remove vertices
	// to implement “defect” mergers.  Instead all edges of the corresponding vertex are
	// removed via `clear_vertex()`.

	for (auto merger : iter_all<DNCMergerOnHICANN>()) {
		auto vertex = add_vertex(Merger{4, merger.value()}, m_graph);
		assert(operator[](merger) == vertex);
	}

#define ADD_VERTICES(LEVEL)                                                                        \
	for (auto merger : iter_all<BOOST_PP_CAT(BOOST_PP_CAT(Merger, LEVEL), OnHICANN)>()) {          \
		auto vertex = add_vertex(Merger{LEVEL, merger.value()}, m_graph);                          \
		assert(operator[](merger) == vertex);                                                      \
	}

	ADD_VERTICES(3);
	ADD_VERTICES(2);
	ADD_VERTICES(1);
	ADD_VERTICES(0);
#undef ADD_VERTICES

#define ADD_EDGES(LEVEL)                                                                           \
	for (auto merger : iter_all<BOOST_PP_CAT(BOOST_PP_CAT(Merger, LEVEL), OnHICANN)>()) {          \
		add_edge(operator[](merger), operator[](merger.inputs().left()), left, m_graph);           \
		add_edge(operator[](merger), operator[](merger.inputs().right()), right, m_graph);         \
	}

	ADD_EDGES(3);
	ADD_EDGES(2);
	ADD_EDGES(1);
#undef ADD_EDGES

	// add edges from DNCMerger to normal mergers
	add_edge(operator[](DNCMergerOnHICANN(0)), operator[](Merger0OnHICANN(0)), right, m_graph);
	add_edge(operator[](DNCMergerOnHICANN(1)), operator[](Merger1OnHICANN(0)), right, m_graph);
	add_edge(operator[](DNCMergerOnHICANN(2)), operator[](Merger0OnHICANN(2)), right, m_graph);
	add_edge(operator[](DNCMergerOnHICANN(3)), operator[](Merger3OnHICANN(0)), right, m_graph);
	add_edge(operator[](DNCMergerOnHICANN(4)), operator[](Merger0OnHICANN(4)), right, m_graph);
	add_edge(operator[](DNCMergerOnHICANN(5)), operator[](Merger2OnHICANN(1)), right, m_graph);
	add_edge(operator[](DNCMergerOnHICANN(6)), operator[](Merger1OnHICANN(3)), right, m_graph);
	add_edge(operator[](DNCMergerOnHICANN(7)), operator[](Merger0OnHICANN(7)), right, m_graph);
}

auto MergerTreeGraph::operator[](DNCMergerOnHICANN const& merger) const -> vertex_descriptor
{
	return {merger.value()};
}

auto MergerTreeGraph::operator[](Merger3OnHICANN const& merger) const -> vertex_descriptor
{
	return {DNCMergerOnHICANN::size + merger.value()};
}

auto MergerTreeGraph::operator[](Merger2OnHICANN const& merger) const -> vertex_descriptor
{
	return {DNCMergerOnHICANN::size + Merger3OnHICANN::size + merger.value()};
}

auto MergerTreeGraph::operator[](Merger1OnHICANN const& merger) const -> vertex_descriptor
{
	return {DNCMergerOnHICANN::size + Merger3OnHICANN::size + Merger2OnHICANN::size +
	        merger.value()};
}

auto MergerTreeGraph::operator[](Merger0OnHICANN const& merger) const -> vertex_descriptor
{
	return {DNCMergerOnHICANN::size + Merger3OnHICANN::size + Merger2OnHICANN::size +
	        Merger1OnHICANN::size + merger.value()};
}

auto MergerTreeGraph::graph() -> graph_type&
{
	return m_graph;
}

auto MergerTreeGraph::graph() const -> graph_type const&
{
	return m_graph;
}

} // namespace placement
} // namespace marocco
