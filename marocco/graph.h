#pragma once

#include <iosfwd>

// boost PBGL
#include <boost/graph/use_mpi.hpp>
#include <boost/graph/distributed/adjacency_list.hpp>
#include <boost/graph/distributed/mpi_process_group.hpp>

// the PyNN stuff
#ifndef PYPLUSPLUS
#include <euter/interface.h>
#else
struct Population {};
struct PopulationPtr {};
struct ProjectionView {};
#endif

namespace marocco {

// http://www.boost.org/doc/libs/1_49_0/libs/graph/doc/using_adjacency_list.html#sec:custom-edge-properties

struct population_t
{
	typedef boost::vertex_property_tag kind;
};

struct projection_t
{
	typedef boost::edge_property_tag kind;
};

struct vertex_size_t
{
	typedef boost::vertex_property_tag kind;
};


typedef boost::property<population_t, PopulationPtr> vertex_property_t;
typedef boost::property<projection_t, ProjectionView> edge_property_t;

typedef boost::adjacency_list<          // the boost graph
	boost::vecS,                        // out edges are std::vectors
	boost::vecS,
	boost::bidirectionalS,              // graph is directed
	vertex_property_t,                  // vertices are populations
	edge_property_t> graph_t;           // edges are projections

bool is_source(graph_t::vertex_descriptor const& v, graph_t const& graph);
bool is_physical(graph_t::vertex_descriptor const& v, graph_t const& graph);

bool is_local_edge(graph_t::edge_descriptor const& e, graph_t const& graph);
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
