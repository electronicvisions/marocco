#pragma once

#include <random>

#include <boost/functional/hash.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <unordered_map>

#include "hal/Coordinate/HICANN.h"
#include "hal/Coordinate/L1.h"
#include "hal/Coordinate/typed_array.h"
#include "marocco/routing/L1BusOnWafer.h"
#include "marocco/routing/parameters/L1Routing.h"
#include "marocco/util.h"

namespace marocco {
namespace routing {

class PathBundle;

/**
 * @brief Representation of the layer 1 routing of a single wafer.
 */
class L1RoutingGraph
{
public:
	L1RoutingGraph(parameters::L1Routing l1_routing_parameters = parameters::L1Routing())
	    : m_l1_routing_parameters(l1_routing_parameters)
	    , m_random_engine_shuffle_switches(m_l1_routing_parameters.shuffle_switches_seed())
	{}

	typedef L1BusOnWafer value_type;
	typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::undirectedS, value_type>
	    graph_type;
	typedef graph_type::vertex_descriptor vertex_descriptor;
	typedef graph_type::edge_descriptor edge_descriptor;

	class HICANN
	{
	public:
		HICANN(
		    graph_type& graph,
		    HMF::Coordinate::HICANNOnWafer const& hicann,
		    parameters::L1Routing::SwitchOrdering switch_ordering,
		    std::default_random_engine& random_engine);

		vertex_descriptor operator[](HMF::Coordinate::HLineOnHICANN const& hline) const;
		vertex_descriptor operator[](HMF::Coordinate::VLineOnHICANN const& vline) const;

	private:
		template <typename V, typename K>
		using typed_array = HMF::Coordinate::typed_array<V, K>;
		typed_array<vertex_descriptor, HMF::Coordinate::HLineOnHICANN> m_horizontal;
		typed_array<vertex_descriptor, HMF::Coordinate::VLineOnHICANN> m_vertical;
	}; // HICANN

	graph_type& graph();
	graph_type const& graph() const;

	void add(HMF::Coordinate::HICANNOnWafer const& hicann);

	void remove(PathBundle const& bundle);
	void remove(
		HMF::Coordinate::HICANNOnWafer const& hicann,
		HMF::Coordinate::HLineOnHICANN const& hline);
	void remove(
	    HMF::Coordinate::HICANNOnWafer const& hicann,
	    HMF::Coordinate::VLineOnHICANN const& vline);
	void remove(
		HMF::Coordinate::HICANNOnWafer const& hicann,
		HMF::Coordinate::HRepeaterOnHICANN const& hrep);
	void remove(
		HMF::Coordinate::HICANNOnWafer const& hicann,
		HMF::Coordinate::VRepeaterOnHICANN const& vrep);

	/**
	 * @throw ResourceNotPresentError when HICANN has not been added yet.
	 */
	HICANN const& operator[](HMF::Coordinate::HICANNOnWafer const& hicann) const;

	value_type& operator[](vertex_descriptor vertex);

	value_type const& operator[](vertex_descriptor vertex) const;

	/**
	 * @throw ResourceNotPresentError when corresponding HICANN has not been added yet.
	 */
	vertex_descriptor operator[](value_type bus) const;

private:
	/**
	 * @brief Connect given HICANN to adjacent HICANN in graph (if present).
	 * @return Whether there was a HICANN to connect to.
	 */
	template <typename LineT>
	bool connect(
		HMF::Coordinate::HICANNOnWafer const& hicann,
		HMF::Coordinate::HICANNOnWafer (HMF::Coordinate::HICANNOnWafer::*conv)() const,
		LineT (LineT::*line_conv)() const);

	graph_type m_graph;
	std::unordered_map<HMF::Coordinate::HICANNOnWafer, HICANN> m_hicanns;
	parameters::L1Routing m_l1_routing_parameters;
	std::default_random_engine m_random_engine_shuffle_switches;
}; // L1RoutingGraph

} // namespace routing
} // namespace marocco

namespace std {

template<>
struct hash<marocco::routing::L1RoutingGraph::edge_descriptor>
{
	size_t operator()(marocco::routing::L1RoutingGraph::edge_descriptor const& e) const
	{
		size_t hash = 0;
		boost::hash_combine(hash, e.m_source);
		boost::hash_combine(hash, e.m_target);
		return hash;
	}
};

} // namespace std
