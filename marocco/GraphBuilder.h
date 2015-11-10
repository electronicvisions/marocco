#pragma once

#include "marocco/graph.h"

#include <type_traits>
#include <functional>
#include <unordered_map>
#include <string>

namespace marocco {

class GraphBuilder
{
public:
	typedef std::unordered_map<
		Population const*, graph_t::vertex_descriptor> VertexMap;

	GraphBuilder(graph_t& graph);
	~GraphBuilder();

	void build(ObjectStore const& os);

	VertexMap const& vertex_mapping() const;

	/* filename of graphviz bio graph output (including suffix, e.g. ".dot")
	   must be called after graph is built
	 */
	void write_bio_graph(std::string const& filename) const;

private:
	graph_t& mrg;

	std::vector<std::reference_wrapper<Population const>> mPopulationList;
	std::vector<std::reference_wrapper<Projection const>> mProjectionList;

	// map to keep track of already inserted populations
	VertexMap mVertexMapping;
};

} // namespace marocco
