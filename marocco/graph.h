#pragma once

#include <iosfwd>
#include <boost/graph/adjacency_list.hpp>

// the PyNN stuff
#ifndef PYPLUSPLUS
#include <euter/interface.h>
#else
struct Population {};
struct PopulationPtr {};
struct ProjectionView {};
#endif

namespace marocco {

typedef boost::adjacency_list< // the boost graph
	boost::vecS,			   // out edges are std::vectors
	boost::vecS,
	boost::bidirectionalS,   // graph is directed
	PopulationPtr,			 // vertices are populations
	ProjectionView> graph_t; // edges are projections

bool is_source(graph_t::vertex_descriptor const& v, graph_t const& graph);
bool is_physical(graph_t::vertex_descriptor const& v, graph_t const& graph);

bool is_spikeinput_edge(graph_t::edge_descriptor const& e, graph_t const& graph);

} // namespace marocco

namespace std {

template<>
struct hash<marocco::graph_t::edge_descriptor>
{
	size_t operator()(marocco::graph_t::edge_descriptor const& e) const
	{
		size_t hash = 0;
		boost::hash_combine(hash, e.m_source);
		boost::hash_combine(hash, e.m_target);
		return hash;
	}
};

} // namespace std
