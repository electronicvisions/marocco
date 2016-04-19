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

void BioGraph::write_graphviz(std::string const& filename) const
{
	// try to open file
	std::ofstream file(filename.c_str());
	if (!file.is_open()) {
        throw std::runtime_error("unable to open file " + filename);
	}

	auto population_label_writer = [this](std::ostream& out, vertex_descriptor const& v) {
		auto p = m_graph[v];
		out << "[label=\"" << p->size() << " " << getCellTypeName(p->type()) << "\"]";
	};

	auto projection_label_writer = [this](std::ostream& out, edge_descriptor const& e) {
		out << "[label=\"";
		m_graph[e].projection()->printOn(out);
		out << "\"]";
	};

	boost::write_graphviz(file, m_graph, population_label_writer, projection_label_writer);
}

bool is_source(BioGraph::vertex_descriptor const& v, BioGraph::graph_type const& graph) {
	return graph[v]->parameters().is_source();
}

bool is_physical(BioGraph::vertex_descriptor const& v, BioGraph::graph_type const& graph) {
	return !graph[v]->parameters().is_source();
}

} // namespace marocco
