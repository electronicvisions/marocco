#pragma once

#include <set>
#include <unordered_map>
#include <vector>

#include <boost/optional.hpp>

#include "hal/Coordinate/L1.h"

#include "marocco/BioGraph.h"
#include "marocco/config.h"
#include "marocco/coordinates/L1Route.h"
#include "marocco/coordinates/L1RouteTree.h"
#include "marocco/placement/results/Placement.h"
#include "marocco/resource/Manager.h"
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
	    results::L1Routing& result,
	    resource::HICANNManager& resource_manager);

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
	resource::HICANNManager& m_resource_manager;
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

/**
 * @brief function to discard branching candidates if too many L1-crossbar-switches are going to be
 * set.
 * @param[in] switch_from : the source bus
 * @param[in] switch_to_candidates : vector of target buses to test
 * @param[in] predecessors : according to its content the decisions are made
 * @param[in] res_mgr : the resource manager, used to load calibration for the L1Crossbar switches
 * @param[in] l1_graph : graph used to convert vertex_descriptors to L1BusOnWafer
 * @return : returns a vector with allowed target buses
 */
std::vector<L1RoutingGraph::vertex_descriptor> L1_crossbar_restrictioning(
    L1RoutingGraph::vertex_descriptor const& switch_from,
    std::vector<L1RoutingGraph::vertex_descriptor> const& switch_to_candidates,
    std::vector<L1RoutingGraph::vertex_descriptor> const& predecessors,
    boost::optional<resource::HICANNManager>
        res_mgr, // might load config, thus prevents the whole function from being const
    L1RoutingGraph::graph_type const& l1_graph);

} // namespace routing
} // namespace marocco
