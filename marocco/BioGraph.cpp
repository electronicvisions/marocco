#include "marocco/BioGraph.h"

#include <boost/bind.hpp>
#include <boost/graph/graphviz.hpp>
#include <boost/ref.hpp>

namespace marocco {

void BioGraph::load(ObjectStore const& os)
{
	for (auto pop : os.populations()) {
		if (m_vertices.find(pop.get()) == m_vertices.end()) {
			graph_type::vertex_descriptor v =
			    add_vertex(boost::const_pointer_cast<const Population>(pop), m_graph);
			// Conveniently the vertex descriptor (which is used in the placement result
			// container) coincides with the euter id of the population because of the
			// order of iteration.  Thus we do not need an additional lookup table.
			assert(v == pop->id());
			m_vertices[pop.get()] = v;
		}
	}

	for (auto proj : os.projections()) {
		for (auto proj_view : proj->flatten()) {
			auto edge = add_edge(
			    m_vertices.at(proj_view.pre().population_ptr().get()),
			    m_vertices.at(proj_view.post().population_ptr().get()), proj_view, m_graph);

			if (!edge.second) {
				std::runtime_error("could not build tree");
			}

			m_edges.insert(edges_type::value_type(edge.first, m_edges.size()));
		}
	}
}

auto BioGraph::operator[](Population const* pop) const -> vertex_descriptor
{
	return m_vertices.at(pop);
}

auto BioGraph::graph() -> graph_type&
{
	return m_graph;
}

auto BioGraph::graph() const -> graph_type const&
{
	return m_graph;
}

routing::results::Edge BioGraph::edge_to_id(edge_descriptor const& edge) const
{
	return routing::results::Edge(m_edges.left.at(edge));
}

auto BioGraph::edge_from_id(routing::results::Edge const& id) const -> edge_descriptor
{
	return m_edges.right.at(id.value());
}

bool is_source(BioGraph::vertex_descriptor const& v, BioGraph::graph_type const& graph) {
	return graph[v]->parameters().is_source();
}

bool is_physical(BioGraph::vertex_descriptor const& v, BioGraph::graph_type const& graph) {
	return !graph[v]->parameters().is_source();
}

} // namespace marocco
