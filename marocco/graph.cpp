#include "marocco/graph.h"
#include <ostream>

namespace marocco {

bool is_source(graph_t::vertex_descriptor const& v, graph_t const& graph) {
	auto const& pop = get(population_t(), graph);
	return pop[v]->parameters().is_source();
}

// check if vertex is no source and local
bool is_physical(graph_t::vertex_descriptor const& v, graph_t const& graph) {
	auto pop = get(population_t(), graph);
	return !pop[v]->parameters().is_source();
}

bool is_local_edge(graph_t::edge_descriptor const& e, graph_t const& graph) {
	return true;
	//return source(e, graph).owner == target(e, graph).owner;
}

bool is_spikeinput_edge(graph_t::edge_descriptor const& e, graph_t const& graph) {
	return is_source(source(e, graph), graph);
}

} // namespace marocco

namespace std {

//std::ostream& operator<< (std::ostream& os, marocco::graph_t::vertex_descriptor const& v)
//{
	//return os << "vertex(" << v.owner << ", " << v.local << ")";
//}

//std::ostream& operator<< (std::ostream& os, marocco::graph_t::edge_descriptor const& e)
//{
	//return os << "edge("
		//<< e.source_processor << ", "
		//<< e.target_processor << ", "
		//<< e.source_owns_edge << ", "
		//<< e.local << ")";
//}

} // namespace std
