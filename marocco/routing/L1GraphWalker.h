#pragma once

#include <set>
#include <vector>
#include <utility>

#include "marocco/routing/L1RoutingGraph.h"

namespace marocco {
namespace routing {

/**
 * @brief Encapsulates traversal of the L1 routing graph across HICANN boundaries.
 */
class L1GraphWalker {
public:
	typedef L1RoutingGraph::graph_type graph_type;
	typedef L1RoutingGraph::vertex_descriptor vertex_descriptor;
	typedef L1RoutingGraph::edge_descriptor edge_descriptor;
	typedef std::vector<vertex_descriptor> path_type;

	L1GraphWalker(graph_type const& graph);

	/**
	 * @brief Ensure that some vertex is not utilized.
	 * This can be used to prevent usage of valuable SPL1 buses.
	 */
	void avoid_using(vertex_descriptor const& vertex);

	/**
	 * @brief Finds the vertex that corresponds to stepping to an adjacent HICANN.
	 * @param[in,out] vertex Vertex descriptor of the starting point.  If step succeeds
	 *                       this is updated to the destination.
	 * @return Whether stepping in the specified direction was possible.
	 * @throw std::invalid_argument If the orientation of \c vertex does not allow walking
	 *                              in the specified direction.
	 */
	bool step(vertex_descriptor& vertex, HMF::Coordinate::Direction const& direction) const;

	/**
	 * @brief Returns all connected vertices with different orientation
	 *        in regard to the given vertex.
	 */
	std::vector<vertex_descriptor> change_orientation(vertex_descriptor const& vertex) const;

	/**
	 * @brief Walk in the specified direction until a limiting HICANN coordinate is reached.
	 * @param limit Value of the HICANN coordinate corresponding to the specified direction.
	 *              Note that this limit is inclusive.
	 * @return Pair of all vertices visited during the walk (excluding \c vertex) and a
	 *         flag encoding whether the limit was reached.
	 * @note If the limit can not be reached by walking at least one step in the specified
	 *       direction the returned path will be empty and the returned flag will be true.
	 * @throw std::invalid_argument If the orientation of \c vertex does not allow walking
	 *                              in the specified direction.
	 */
	std::pair<path_type, bool> walk(
	    vertex_descriptor const& vertex,
	    HMF::Coordinate::Direction const& direction,
	    size_t limit) const;

	/**
	 * @brief Take a detour perpendicular to the specified direction then walk in the
	 *        specified direction until a limiting HICANN coordinate is reached.
	 * @see #walk() for a description of arguments, return type and behavior in case of
	 *      unreachable limits.
	 * The algorithm considers all connected buses with different orientation and,
	 * starting from \c vertex, steps to other HICANNs in the perpendicular direction.
	 * On each HICANN it tries to walk in the original direction again, as far as
	 * possible.  If \c limit is reached, the algorithm returns immediately.
	 * Else the search is continued for all possible HICANNs and the detour with
	 * the longest extension in the original direction is chosen.
	 * @invariant If the returned path is non-empty, the detour has a non-zero length
	 *            along the original direction.
	 * @throw std::invalid_argument If the orientation of \c vertex does not allow walking
	 *                              in the specified direction.
	 */
	std::pair<path_type, bool> detour_and_walk(
	    vertex_descriptor const& vertex,
	    HMF::Coordinate::Direction const& direction,
	    size_t limit) const;

	graph_type const& graph() const;

private:
	typedef HMF::Coordinate::HICANNOnWafer::x_type x_type;
	typedef HMF::Coordinate::HICANNOnWafer::y_type y_type;

	std::pair<path_type, bool> walk_north(
		vertex_descriptor const& vertex, y_type const& limit) const;

	std::pair<path_type, bool> walk_east(
		vertex_descriptor const& vertex, x_type const& limit) const;

	std::pair<path_type, bool> walk_south(
		vertex_descriptor const& vertex, y_type const& limit) const;

	std::pair<path_type, bool> walk_west(
		vertex_descriptor const& vertex, x_type const& limit) const;

	graph_type const& m_graph;
	std::set<vertex_descriptor> m_avoid;
}; // L1GraphWalker

} // namespace routing
} // namespace marocco
