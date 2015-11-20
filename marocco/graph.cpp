#include "marocco/graph.h"
#include <ostream>

namespace marocco {

bool is_source(graph_t::vertex_descriptor const& v, graph_t const& graph) {
	return graph[v]->parameters().is_source();
}

bool is_physical(graph_t::vertex_descriptor const& v, graph_t const& graph) {
	return !is_source(v, graph);
}

bool is_spikeinput_edge(graph_t::edge_descriptor const& e, graph_t const& graph) {
	return is_source(source(e, graph), graph);
}

} // namespace marocco
