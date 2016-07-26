#pragma once

#include <string>
#ifndef PYPLUSPLUS
#include <unordered_map>
#endif // !PYPLUSPLUS
#include <boost/bimap.hpp>
#include <boost/graph/adjacency_list.hpp>

#ifndef PYPLUSPLUS
#include "euter/interface.h"
#else
class ObjectStore {};
class Population {};
class ConstPopulationPtr {};
class ProjectionView {};
#endif // !PYPLUSPLUS

#include "marocco/util/iterable.h"
#include "marocco/routing/results/Edge.h"

namespace marocco {

/**
 * @brief Representation of the biological network.
 * The biological network description is converted into a directed graph, with populations
 * as vertices and projection views as edges.
 */
class BioGraph {
public:
	typedef boost::adjacency_list<boost::vecS,
	                              boost::vecS,
	                              boost::bidirectionalS,
	                              ConstPopulationPtr,
	                              ProjectionView>
	    graph_type;
	typedef graph_type::vertex_descriptor vertex_descriptor;
	typedef graph_type::edge_descriptor edge_descriptor;
#ifndef PYPLUSPLUS
	typedef std::unordered_map<Population const*, vertex_descriptor> vertices_type;
	typedef boost::bimap<edge_descriptor, size_t> edges_type;
#endif // !PYPLUSPLUS

	void load(ObjectStore const& os);

	/**
	 * @brief Return vertex descriptor for the specified population.
	 * @throw std::out_of_range If the population is not contained in this graph.
	 */
	vertex_descriptor operator[](Population const* pop) const;

	graph_type& graph();
	graph_type const& graph() const;

	routing::results::Edge edge_to_id(edge_descriptor const& edge) const;

	edge_descriptor edge_from_id(routing::results::Edge const& id) const;

	/**
	 * @brief Export graph in graphviz format.
	 * @throw std::runtime_error If the specified file could not be opened.
	 */
	void write_graphviz(std::string const& filename) const;

private:
#ifndef PYPLUSPLUS
	graph_type m_graph;
	vertices_type m_vertices;
	edges_type m_edges;
#endif // !PYPLUSPLUS
}; // BioGraph

bool is_source(BioGraph::vertex_descriptor const& v, BioGraph::graph_type const& graph);
bool is_physical(BioGraph::vertex_descriptor const& v, BioGraph::graph_type const& graph);

} // namespace marocco

namespace std {

template<>
struct hash<marocco::BioGraph::edge_descriptor>
{
	size_t operator()(marocco::BioGraph::edge_descriptor const& e) const
	{
		size_t hash = 0;
		// We could also just use e.m_eproperty instead.
		boost::hash_combine(hash, e.m_source);
		boost::hash_combine(hash, e.m_target);
		return hash;
	}
};

} // namespace std
