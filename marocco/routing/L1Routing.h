#pragma once

#include <set>
#include <unordered_map>
#include <vector>

#include "hal/Coordinate/L1.h"

#include "marocco/BioGraph.h"
#include "marocco/coordinates/L1Route.h"
#include "marocco/coordinates/L1RouteTree.h"
#include "marocco/placement/results/Placement.h"
#include "marocco/routing/L1RoutingGraph.h"
#include "marocco/routing/PathBundle.h"
#include "marocco/routing/parameters/L1Routing.h"
#include "marocco/routing/results/L1Routing.h"

namespace marocco {
namespace routing {

class L1Routing {
public:
	typedef std::unordered_map<HMF::Coordinate::HICANNOnWafer, std::set<BioGraph::edge_descriptor> >
		targets_type;

	struct request_type
	{
		HMF::Coordinate::DNCMergerOnWafer source;
		HMF::Coordinate::HICANNOnWafer target;
		std::set<BioGraph::edge_descriptor> projections;
	}; // request_type

	L1Routing(
		L1RoutingGraph& l1_graph,
		BioGraph const& bio_graph,
		parameters::L1Routing const& parameters,
		placement::results::Placement const& neuron_placement,
		results::L1Routing& result);

	/**
	 * @brief Run routing algorithm.
	 * @note This modifies the L1 graph.
	 */
	void run();

	/**
	 * @brief Returns a sequence of mergers, sorted by the priority of efferent projections.
	 * @return Sequence of DNC mergers without duplicate entries.
	 */
	std::vector<HMF::Coordinate::DNCMergerOnWafer> sources_sorted_by_priority() const;

	std::vector<request_type> const& failed_routes() const;

private:
	void run_backbone_router();
	void run_dijkstra_router();
	bool store_result(
		request_type const& request,
	    L1RoutingGraph::vertex_descriptor const source,
	    PathBundle::path_type const& path);

	L1RoutingGraph& m_l1_graph;
	BioGraph const& m_bio_graph;
	parameters::L1Routing const& m_parameters;
	placement::results::Placement const& m_neuron_placement;
	results::L1Routing& m_result;
	std::vector<request_type> m_failed;
}; // L1Routing

/**
 * @brief Create an \c L1Route from a sequence of routing graph vertices.
 * @note If you want sending repeaters to be configured you have to prepend a
 *       \c DNCMergerOnHICANN segment.
 * @throw InvalidRouteError if \c path does not represent a well-formed route.
 */
L1Route toL1Route(PathBundle::graph_type const& graph, PathBundle::path_type const& path);
L1RouteTree toL1RouteTree(PathBundle::graph_type const& graph, PathBundle const& bundle);

/**
 * @brief Prepend DNC merger information to route.
 * This is required to properly configure the sending repeaters.
 * @throw InvalidRouteError When this leads to invalid routes.
 */
L1Route with_dnc_merger_prefix(
    L1Route const& route, HMF::Coordinate::DNCMergerOnWafer const& merger);

} // namespace routing
} // namespace marocco
