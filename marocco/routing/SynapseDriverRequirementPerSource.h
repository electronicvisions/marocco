#pragma once

// std header
#include <set>
#include <unordered_map>

// custom header
#include "marocco/config.h"
#include "marocco/graph.h"
#include "halco/hicann/v2/l1.h"

// forward declerations
namespace marocco {
namespace placement {
namespace results {
class Placement;
}
}
}

namespace halco {
namespace hicann {
namespace v2 {
class DNCMergerOnWafer;
class HICANNOnWafer;
}
}
}

namespace HMF {
namespace Coordinate {
typedef halco::hicann::v2::HICANNOnWafer HICANNOnWafer;
typedef halco::hicann::v2::DNCMergerOnWafer DNCMergerOnWafer;
}
}

// class header
namespace marocco {
namespace routing {

class SynapseDriverRequirementPerSource
{
public:

	/**
	 * Constructor for SynapseDriver Requirements
	 *
	 * after instanciation, with graph and placement, different mergers can be
	 * queried for their targets, driver requirements and potentially more.
	 *
	 * @param [in] bio_graph: to calculate the reqirements on
	 * @param [in] placement: the placement to consider for calculations
	 */
	SynapseDriverRequirementPerSource(
	    graph_t const& bio_graph,
		placement::results::Placement const& placement);

	/**
	 * @brief Returns efferent projections of the given merger, grouped by their target.
	 */
	std::unordered_map<halco::hicann::v2::HICANNOnWafer, std::set<BioGraph::edge_descriptor> >
	targets_for_source(halco::hicann::v2::DNCMergerOnWafer const& merger) const;

	/**
	 * @brief For a given Merger it returs the maximum Drivers required over
	 * all Targeted Synapses arrays.
	 *
	 * if it requires 4 and 2 Drivers in 2 different synapse arrays, 4 is returned.
	 */
	size_t drivers(halco::hicann::v2::DNCMergerOnWafer const& merger) const;

	/**
	 * @brief returns true if driver reqiurements for all targets can be matched
	 */
	bool drivers_possible(
	    halco::hicann::v2::DNCMergerOnWafer const& merger, resource::HICANNManager const& mgr) const;

	/**
	 * @brief returns true if driver reqiurements for all targets can be matched with ease
	 */
	bool more_drivers_possible(
	    halco::hicann::v2::DNCMergerOnWafer const& merger, resource::HICANNManager const& mgr) const;

private:
	graph_t const& m_bio_graph;
	placement::results::Placement const& m_placement;
	mutable std::unordered_map<halco::hicann::v2::DNCMergerOnWafer, std::unordered_map<halco::hicann::v2::HICANNOnWafer, std::set<BioGraph::edge_descriptor> > > m_cached_results;
	mutable std::unordered_map<halco::hicann::v2::DNCMergerOnWafer, std::unordered_map<halco::hicann::v2::HICANNOnWafer, std::set<BioGraph::edge_descriptor> > > m_cached_targets;
	mutable std::unordered_map<halco::hicann::v2::DNCMergerOnWafer, size_t> m_cached_drivers;

	/**
	 * @brief a map HICANN -> set(edge) is retured
	 *
	 * if it is cached, it is loaded from cache, if not it is calclulated.
	 *
	 * @param [in] merger : the merger the targets are searched for
	 */
	std::unordered_map<halco::hicann::v2::HICANNOnWafer, std::set<BioGraph::edge_descriptor> >
	fill_results(halco::hicann::v2::DNCMergerOnWafer const& merger) const;

	/**
	 * @brief precalculates values and stores them in cache.
	 *
	 * calculates m_cached_targets, m_cached_drivers.
	 *
	 * @param [in] the merger, values shall be calculated for
	 */
	void precalc(halco::hicann::v2::DNCMergerOnWafer const& merger) const;
};

} // namespace routing
} // namespace marocco
