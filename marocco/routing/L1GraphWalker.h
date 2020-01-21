#pragma once

#include <set>
#include <vector>
#include <utility>

#include "marocco/config.h"
#include "marocco/resource/Manager.h"
#include "marocco/routing/L1Routing.h"
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

	/**
	 * Constructor
	 *
	 * @param [in] graph : graph for routing
	 * @param [in] resource_manager : used to restrict routing on the graph (hardware constraints)
	 */
	L1GraphWalker(
	    graph_type const& graph,
	    boost::optional<resource::HICANNManager&> resource_manager = boost::none);

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
	bool step(vertex_descriptor& vertex, halco::common::Direction const& direction) const;

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
	    halco::common::Direction const& direction,
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
	 *
	 * @param [in] vertex : the vertex to start the detour from
	 * @param [in] direction : in which direction shall it try to extend
	 * @param [in] limit : the coordinate to stop
	 * @param [in, optional] predecessors : a predecessor list, used to restrict switch usage to
	 * allowed configurations
	 *
	 */
	std::pair<path_type, bool> detour_and_walk(
	    vertex_descriptor const& vertex,
	    halco::common::Direction const& direction,
	    size_t limit) const;

	std::pair<path_type, bool> detour_and_walk(
	    vertex_descriptor const& vertex,
	    halco::common::Direction const& direction,
	    size_t limit,
	    path_type predecessors) const;

	graph_type const& graph() const;

private:
	typedef halco::hicann::v2::HICANNOnWafer::x_type x_type;
	typedef halco::hicann::v2::HICANNOnWafer::y_type y_type;

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

	boost::optional<resource::HICANNManager&> m_res_mgr;
}; // L1GraphWalker

} // namespace routing
} // namespace marocco
