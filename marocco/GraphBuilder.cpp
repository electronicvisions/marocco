#include <stdexcept>
#include <unordered_map>

#include "marocco/GraphBuilder.h"
#include "marocco/util.h"
#include "marocco/Logger.h"

#include <boost/graph/graphviz.hpp>

namespace marocco {

struct population_label_writer {
	population_label_writer(const graph_t& g) : g(g) {}

	template <class VertexOrEdge>
	void operator()(std::ostream& out, const VertexOrEdge& v) const {
		auto p = g[v];
		out << "[label=\"" << p->size() << " " << getCellTypeName(p->type()) << "\"]";
	}

	graph_t g;
};

struct projection_label_writer {
	projection_label_writer(const graph_t& g) : g(g) {}

	template <class VertexOrEdge>
	void operator()(std::ostream& out, const VertexOrEdge& v) const {
		auto p = g[v];

		out << "[label=\"";
		p.projection()->printOn(out);
		out << "\"]";
	}

	graph_t g;
};

GraphBuilder::GraphBuilder(graph_t& graph) :
	mrg(graph) {}

GraphBuilder::~GraphBuilder() {}

void GraphBuilder::build(ObjectStore const& os)
{
	// insert all populations into map
	mrg.m_vertices.reserve(os.populations().size());
	for(auto pop : os.populations())
	{
		if (mVertexMapping.find(pop.get())==mVertexMapping.end()) {
			graph_t::vertex_descriptor v =
				add_vertex(boost::const_pointer_cast<Population>(pop), mrg);
			mVertexMapping[pop.get()] = v;
		}
	}

	for(auto proj : os.projections())
	{
		for(auto proj_view : proj->flatten())
		{
			std::pair<typename graph_t::edge_descriptor, bool> edge = add_edge(
				mVertexMapping.at(proj_view.pre().population_ptr().get()),
				mVertexMapping.at(proj_view.post().population_ptr().get()),
				proj_view, mrg);

			if (!edge.second)
				std::runtime_error("could not build tree");

		} // for all projection_views
	} // for all projections
}

GraphBuilder::VertexMap const& GraphBuilder::vertex_mapping() const
{
	return mVertexMapping;
}

void GraphBuilder::write_bio_graph(std::string const& filename) const {

	// try to open file
	std::ofstream file(filename.c_str());
	if (!file.is_open()) {
        throw std::runtime_error("unable to open file " + filename);
	}

	write_graphviz(file, mrg, population_label_writer(mrg), projection_label_writer(mrg));
}

} // namespace marocco
