#pragma once

#include <functional>
#include <map>
#include <unordered_map>

#include <boost/optional.hpp>

#include "marocco/config.h"
#include "marocco/resource/Manager.h"
#include "marocco/routing/L1GraphWalker.h"
#include "marocco/routing/L1Routing.h"
#include "marocco/routing/L1RoutingGraph.h"
#include "marocco/routing/PathBundle.h"

namespace marocco {
namespace routing {

/**
 * @brief Iterative horizontal growth routing for L1 buses.
 * Starting from a vertex corresponding to a horizontal bus, this algorithm will try to
 * establish routes to vertical buses on target HICANNs by creating two horizontal
 * “backbones” (one westwards, one eastwards) from which vertical “ribs” emerge.
 * When routing the backbones, vertical detours are used if necessary.
 * As there are several possible vertical buses for the start of each vertical rib,
 * a scoring function can be provided to e.g. prefer buses with low utilization.
 */
class L1BackboneRouter
{
public:
	typedef L1RoutingGraph::graph_type graph_type;
	typedef L1RoutingGraph::vertex_descriptor vertex_descriptor;
	typedef L1RoutingGraph::edge_descriptor edge_descriptor;
	typedef HMF::Coordinate::HICANNOnWafer target_type;
	typedef std::function<size_t(vertex_descriptor const&)> score_function_type;

	/**
	 * @param walker Encapsulates walking on the graph, used to specify buses to avoid.
	 *               A reference to the graph this algorithm operates on is extracted from
	 *               this parameter.
	 * @param source Vertex corresponding to the horizontal bus the route should start from.
	 * @param vertical_scoring Function that will be called with vertices corresponding to
	 *                         vertical buses on target chips and should return a positive
	 *                         score.  When selecting the starting vertical bus for a
	 *                         vertical rib, the one that leads to the highest score will
	 *                         win.
	 */
	L1BackboneRouter(
	    L1GraphWalker const& walker,
	    vertex_descriptor const& source,
	    score_function_type const& vertical_scoring = nullptr,
	    boost::optional<resource::HICANNManager&> resource_manager = boost::none);

	/**
	 * @brief Adds target requirement.
	 */
	void add_target(target_type const& target);

	/**
	 * @brief Run algorithm.
	 */
	void run();

	/**
	 * @brief Returns the path from source to the given target.
	 * @return Sequence of vertices, from source to target.  If no such path can be
	 *         recovered because of a cycle in the predecessor map, the returned path will
	 *         start at the location of the cycle.
	 * @pre \c run() has been called.
	 */
	PathBundle::path_type path_to(target_type const& target) const;

	/**
	 * @brief Returns the source vertex of the graph search.
	 */
	vertex_descriptor source() const;

private:
	/**
	 * @brief Establish a connection to vertical targets.
	 * If the specified vertex belongs to a horizontal bus, establish connections to
	 * targets lying to the north/south of the respective HICANN and to targets on the
	 * current HICANN via a vertical rib.  To pick the best option out of several possible
	 * vertical lines, a score is computed based on the reached targets.
	 */
	void maybe_branch_off_to_vertical_targets(vertex_descriptor const& vertex);

	bool is_target(target_type const& hicann) const;

	L1GraphWalker const& m_walker;
	graph_type const& m_graph;
	vertex_descriptor const m_source;

	/**
	 * @brief User-provided function to calculate the score of a target vertical bus.
	 * @see L1BackboneRouter::L1BackboneRouter()
	 */
	score_function_type const m_vertical_scoring;

	/**
	 * @brief Target HICANNs, grouped by their x coordinates.
	 * This is used in the algorithm to calculate the required x limits for the backbone
	 * and look up all targets for a given x coordinate.
	 */
	std::map<HMF::Coordinate::HICANNOnWafer::x_type,
	         std::map<HMF::Coordinate::HICANNOnWafer::y_type, target_type> >
	    m_targets;

	/**
	 * @brief Vertex corresponding to a vertical bus on a given target HICANN.
	 */
	std::unordered_map<target_type, vertex_descriptor> m_vertex_for_targets;

	/**
	 * @brief Predecessor map used to store the path to each target vertex.
	 */
	std::vector<vertex_descriptor> m_predecessors;

	boost::optional<resource::HICANNManager&> m_res_mgr;
}; // L1BackboneRouter

} // namespace routing
} // namespace marocco
