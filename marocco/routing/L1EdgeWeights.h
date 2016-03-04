#pragma once

#include <unordered_map>

#include "marocco/routing/L1RoutingGraph.h"

namespace marocco {
namespace routing {

/**
 * @brief Stores weights for the edges in the L1 routing graph.
 * As L1 buses are represented by vertices in the routing graph, edges correspond to the
 * hardware elements connecting them: Crossbar switches and (sending) repeaters.
 * Weights are positive, non-zero numbers and can be set for both edges and vertices.
 * To calculate the effective weight of an edge, the maximum weight of its vertices is
 * used, but only if the weight of the edge has not been set explicitly.
 */
class L1EdgeWeights
{
public:
	typedef L1RoutingGraph::graph_type graph_type;
	typedef L1RoutingGraph::vertex_descriptor vertex_descriptor;
	typedef L1RoutingGraph::edge_descriptor edge_descriptor;
	typedef size_t weight_type;

	L1EdgeWeights(graph_type const& graph);

	/**
	 * @brief Sets weight for edge in routing graph.
	 * @param weight Non-zero weight to assign to this edge.
	 */
	void set_weight(edge_descriptor const& edge, weight_type weight);

	/**
	 * @brief Sets weight for vertex in routing graph.
	 * @param weight Non-zero weight to assign to this vertex.
	 */
	void set_weight(vertex_descriptor const& vertex, weight_type weight);

	/**
	 * @brief Calculates the effective weight for the specified edge.
	 * If a weight has been set explicitly for this edge via #set_weight(), it is used.
	 * Else, the maximum set weight for the vertices connected by this edge is used.
	 * If neither is present, the minimum possible weight (`1`) is returned.
	 */
	weight_type weight(edge_descriptor const& edge) const;

	graph_type const& graph() const;

private:
	graph_type const& m_graph;
	std::unordered_map<edge_descriptor, weight_type> m_edge_weights;
	std::unordered_map<vertex_descriptor, weight_type> m_vertex_weights;
}; // L1EdgeWeights

} // namespace routing
} // namespace marocco
