#include <stdexcept>
#include <unordered_map>

#include "mpi/config.h"

#include "marocco/GraphBuilder.h"
#include "marocco/util.h"
#include "marocco/Logger.h"

#include <boost/graph/graphviz.hpp>

using namespace MPI;

namespace marocco {

struct population_label_writer {
	population_label_writer(const graph_t& g) : g(g) {}

	template <class VertexOrEdge>
	void operator()(std::ostream& out, const VertexOrEdge& v) const {
		const boost::property_map<graph_t, population_t>::const_type data(
		    boost::get(population_t(), g));
		auto p = boost::get(data, v);
		out << "[label=\"" << p->size() << " " << getCellTypeName(p->type()) << "\"]";
	}

	graph_t g;
};

struct projection_label_writer {
	projection_label_writer(const graph_t& g) : g(g) {}

	template <class VertexOrEdge>
	void operator()(std::ostream& out, const VertexOrEdge& v) const {
		const boost::property_map<graph_t, projection_t>::const_type data(
		    boost::get(projection_t(), g));
		auto p = boost::get(data, v);

		out << "[label=\"";
		p.projection()->printOn(out);
		out << "\"]";
	}

	graph_t g;
};

GraphBuilder::GraphBuilder(graph_t& graph, Intracomm& comm) :
	mrg(graph), mComm(comm) {}

GraphBuilder::~GraphBuilder() {}

bool GraphBuilder::is_master() const
{
	return mComm.Get_rank() == MASTER_PROCESS;
}

void GraphBuilder::build_on_master(ObjectStore const& os)
{
	// insert all populations into map
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

void GraphBuilder::build(ObjectStore const& os)
{
	/// build graph on master node (insert projections and populations)
	if (is_master())
	{
		build_on_master(os);
	}
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
