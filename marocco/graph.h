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


template<typename T>
inline
size_t hash_value(T const& t) {
	return std::hash<T> () (t);
}


namespace std {

template<>
struct hash<marocco::graph_t::edge_descriptor>
{
	size_t operator()(marocco::graph_t::edge_descriptor const& e) const
	{
		size_t hash = e.m_source;
		boost::hash_combine(hash, e.m_target);
		//boost::hash_combine(hash, e.source_processor);
		//boost::hash_combine(hash, e.target_processor);
		return hash;
	}
};

//template<>
//struct hash<marocco::graph_t::vertex_descriptor>
//{
	//size_t operator()(marocco::graph_t::vertex_descriptor const& v) const
	//{
		//size_t hash = v.local;
		//boost::hash_combine(hash, v.owner);
		//return v;
	//}
//};

} // namespace std

namespace tbb {

template<typename T>
struct tbb_hash;

template<>
struct tbb_hash<marocco::graph_t::edge_descriptor>
{
	typedef marocco::graph_t::edge_descriptor type;
	size_t operator() (type const& key) const
	{
#if !defined(PYPLUSPLUS)
		static_assert(sizeof(type)>0, "just to make sure, "
					  "this part is not compiled by py++");
		static const std::hash<type> hash{};
		return hash(key);
#else
		return 42;
#endif
	}
};

} // namespace tbb
