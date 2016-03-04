#pragma once

#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <boost/property_map/property_map.hpp>

#include "marocco/routing/PathBundle.h"
#include "marocco/routing/L1RoutingGraph.h"
#include "marocco/routing/L1EdgeWeights.h"
#include "marocco/routing/Target.h"

namespace marocco {
namespace routing {

/**
 * @brief Given a source L1 bus on some HICANN calculate wafer-local routes to targets on
 *        other HICANNs using Dijkstra's algorithm.
 * Target L1 buses are specified via their HICANN coordinate and orientation (horizontal
 * or vertical).
 */
class L1DijkstraRouter
{
public:
	typedef L1RoutingGraph::graph_type graph_type;
	typedef L1RoutingGraph::vertex_descriptor vertex_descriptor;
	typedef L1RoutingGraph::edge_descriptor edge_descriptor;
	typedef Target target_type;
	typedef std::unordered_set<vertex_descriptor> target_vertices_type;

 	/**
 	 * @param weights Used to calculate egde weights to use in Dijkstra's algorithm.  A
 	 *               reference to the graph this algorithm operates on is extracted from
 	 *               this parameter.
 	 * @param source Vertex corresponding to the bus the route should start from.
	 */
	L1DijkstraRouter(L1EdgeWeights const& weights, vertex_descriptor const& source);

	/**
	 * @brief Adds target requirement.
	 * After running Dijkstra's algorithm via \c run(), the routing graph vertices that
	 * match this requirement can be queried via \c vertices_for().
	 */
	void add_target(target_type const& target);

	/**
	 * @brief Run Dijkstra's algorithm.
	 */
	void run();

	/**
	 * @brief Returns all vertices that fulfill the given target requirement.
	 * @pre \c add_target() has been called for this target prior to \c run().
	 * @throw std::runtime_error when target requirement has not been registered first.
	 */
	target_vertices_type const& vertices_for(target_type const& target) const;

	/**
	 * @brief Returns the path from source to target vertex.
	 * @pre \c run() has been called.
	 */
	PathBundle::path_type path_to(vertex_descriptor const& target) const;

	/**
	 * @brief Returns the source vertex of the graph search.
	 */
	vertex_descriptor source() const;

private:
	/**
	 * @brief Called after all out edges of a vertex “have been added to the search tree
	 *        and all of the adjacent vertices have been discovered (but before their
	 *        out-edges have been examined).”
	 * @note This stores all vertices that fulfill target requirements.
	 */
	void finish_vertex(vertex_descriptor const& vertex, graph_type const& graph);

	L1EdgeWeights const& m_weights;
	graph_type const& m_graph;
	vertex_descriptor m_source;
	std::unordered_map<target_type, target_vertices_type> m_targets;
	std::vector<vertex_descriptor> m_predecessors;
	/**
	 * @brief Stores used crossbar switches to avoid multiple switches per line.
	 * @note This only is in effect for paths to vertices belonging to a registered
	 *       target.  Because of the traversal order in Dijkstra's algorithm, nearer
	 *       targets are given preference.
	 * This maps a vertex belonging to a vertical \c L1BusOnWafer, i.e. a \c VLineOnHICANN
	 * with a \c HICANNOnWafer coordinate, to a vertex belonging to an horizontal \c
	 * L1BusOnWafer.  This leads to a HICANN-local “1 switch per vertical line” rule.
	 */
	std::unordered_map<vertex_descriptor, vertex_descriptor> m_used_switches;
}; // L1DijkstraRouter

} // namespace routing
} // namespace marocco
