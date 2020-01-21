#include "SynapseDriverRequirementPerSource.h"

#include "SynapseDriverRequirements.h"
#include "halco/hicann/v2/l1.h"
#include "marocco/Logger.h"
#include "marocco/graph.h"
#include "marocco/placement/results/Placement.h"
#include "marocco/routing/internal/SynapseTargetMapping.h"
#include "marocco/resource/Manager.h"

namespace marocco {
namespace routing {

SynapseDriverRequirementPerSource::SynapseDriverRequirementPerSource(
    graph_t const& bio_graph, placement::results::Placement const& placement)
    : m_bio_graph(bio_graph), m_placement(placement)
{}


std::unordered_map<halco::hicann::v2::HICANNOnWafer, std::set<BioGraph::edge_descriptor> >
SynapseDriverRequirementPerSource::targets_for_source(
    halco::hicann::v2::DNCMergerOnWafer const& merger) const
{
	if( m_cached_targets.find(merger) == m_cached_targets.end() ){
		precalc(merger);
	}
	return m_cached_targets.at(merger);
}

size_t SynapseDriverRequirementPerSource::drivers(
    halco::hicann::v2::DNCMergerOnWafer const& merger) const
{
	if( m_cached_drivers.find(merger) == m_cached_drivers.end() ){
		precalc(merger);
	}

	return m_cached_drivers.at(merger);
}

bool SynapseDriverRequirementPerSource::drivers_possible(
    halco::hicann::v2::DNCMergerOnWafer const& merger, resource::HICANNManager const& mgr) const
{
	if( m_cached_drivers.find(merger) == m_cached_drivers.end() ){
		precalc(merger);
	}

	size_t max_chain_global = 0;
	for(auto const target_with_edges : m_cached_targets.at(merger) ){
		size_t max_chain_local = mgr.getMaxChainLength(target_with_edges.first);
		if(max_chain_local < max_chain_global || max_chain_global == 0){
			max_chain_global = max_chain_local;
		}
	}

	return m_cached_drivers.at(merger) <= max_chain_global;
}

bool SynapseDriverRequirementPerSource::more_drivers_possible(
    halco::hicann::v2::DNCMergerOnWafer const& merger, resource::HICANNManager const& mgr) const
{
	if( m_cached_drivers.find(merger) == m_cached_drivers.end() ){
		precalc(merger);
	}
	if( m_cached_drivers.at(merger) == 0 && m_cached_targets.at(merger).empty() ){
		return true;	// if no drivers are required, and there is no target?
	}

	size_t max_chain_global = 0;
	for(auto const target_with_edges : m_cached_targets.at(merger) ){
		size_t max_chain_local = mgr.getMaxChainLength(target_with_edges.first);
		if(max_chain_local < max_chain_global || max_chain_global == 0){
			max_chain_global = max_chain_local;
		}
	}

	return m_cached_drivers.at(merger) < max_chain_global;
}

std::unordered_map<halco::hicann::v2::HICANNOnWafer, std::set<BioGraph::edge_descriptor> >
SynapseDriverRequirementPerSource::fill_results(
    halco::hicann::v2::DNCMergerOnWafer const& merger) const
{

	if( m_cached_results.find(merger) == m_cached_results.end() ){

		std::unordered_map<halco::hicann::v2::HICANNOnWafer, std::set<BioGraph::edge_descriptor> > result;
		for (auto const& item : m_placement.find(merger)) {
			for (auto const& edge : make_iterable(out_edges(item.population(), m_bio_graph))) {
				auto const target_population = target(edge, m_bio_graph);

				if (is_source(target_population, m_bio_graph)) {
					throw std::runtime_error("target population is a spike source");
				}

				auto const placements = m_placement.find(target_population);
				if (placements.empty()) {
					throw std::runtime_error("target population has not been placed");
				}

				for (auto const& placement : placements) {
					auto const neuron_block = placement.neuron_block();
					// Spike sources should never have incoming edges.
					assert(neuron_block != boost::none);
					result[neuron_block->toHICANNOnWafer()].insert(edge);
				}
			}
		}
		m_cached_results[merger] = result; // store result
	} // else a resltu is already stored
	return m_cached_results.at(merger);
}

void SynapseDriverRequirementPerSource::precalc(
    halco::hicann::v2::DNCMergerOnWafer const& merger) const
{
	auto result = fill_results(merger);
	size_t maxDrivers = 0;

	// Remove HICANN if no outgoing projection has target populations there, i.e. the
	// number of required synapse drivers is zero.
	for (auto it = result.begin(), eit = result.end(); it != eit;) {
		// TODO(#1594): determination whether route has synapes to target does not
		// need to count the total number of synapses.
		routing::results::SynapticInputs synaptic_inputs;
		routing::internal::SynapseTargetMapping::simple_mapping(
			it->first, m_placement, m_bio_graph, synaptic_inputs);
		routing::SynapseDriverRequirements requirements(it->first, m_placement, synaptic_inputs);
		auto const num = requirements.calc(merger, m_bio_graph);

		if (num.first == 0u) {
			it = result.erase(it);
		} else {
			if (num.first > maxDrivers) {
				maxDrivers = num.first;
			}
			++it;
		}
	}
	m_cached_targets[merger] = result;
	m_cached_drivers[merger] = maxDrivers;
}

} // namespace routing
} // namespace marocco
