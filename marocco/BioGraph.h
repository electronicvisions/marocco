#pragma once

#include <string>
#include <boost/bimap.hpp>
#include <boost/unordered_map.hpp>

#ifndef PYPLUSPLUS
#include <boost/graph/adjacency_list.hpp>
#include "euter/interface.h"
#else
// forward declare boost/graph/adjaceny_list.hpp
namespace boost {
template <class VS, class ES, class D, class VT, class ET>
class adjacency_list
{
public:
	typedef VT vertex_descriptor;
	typedef ET edge_descriptor;
};

class vecS;
class bidirectionalS;
}
// forward declare euter/interface.h
class ObjectStore;
class Population;
class ConstPopulationPtr;
class ProjectionView;
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
	typedef boost::unordered_map<Population const*, vertex_descriptor> vertices_type;
	typedef boost::bimap<edge_descriptor, size_t> edges_type;

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

private:
	graph_type m_graph;
#ifndef PYPLUSPLUS
	edges_type m_edges;
	vertices_type m_vertices;
#endif // !PYPLUSPLUS
}; // BioGraph
bool is_source(BioGraph::vertex_descriptor const& v, BioGraph::graph_type const& graph);
bool is_physical(BioGraph::vertex_descriptor const& v, BioGraph::graph_type const& graph);

} // namespace marocco

#ifndef PYPLUSPLUS
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
#endif // !PYPLUSPLUS
