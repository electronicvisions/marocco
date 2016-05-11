#pragma once

#include "hal/Coordinate/HICANN.h"
#include "hal/Coordinate/L1.h"

#include "marocco/BioGraph.h"
#include "marocco/placement/Result.h"
#include "marocco/placement/results/Placement.h"
#include "marocco/routing/L1RoutingGraph.h"
#include "marocco/routing/Result.h"
#include "marocco/routing/SynapseRowSource.h"
#include "marocco/routing/results/L1Routing.h"
#include "marocco/routing/parameters/SynapseRouting.h"

namespace marocco {
namespace routing {

class SynapseLoss;

class SynapseRouting
{
public:
	typedef SynapseRoutingResult::result_type Result;

	SynapseRouting(
		HMF::Coordinate::HICANNGlobal const& hicann,
		BioGraph const& bio_graph,
		hardware_system_t& hardware,
		resource_manager_t& resource_manager,
		parameters::SynapseRouting const& parameters,
		placement::results::Placement const& neuron_placement,
		results::L1Routing const& l1_routing,
		boost::shared_ptr<SynapseLoss> const& synapse_loss);

	void run();

	Result const& getResult() const;
	Result&       getResult();

private:
	void handleSynapseLoss(results::L1Routing::route_item_type const& route_item);

	/// set all synapse decoder to the the 4-bit address disabling the synapse.
	void invalidateSynapseDecoder();

	/// temporariliy tag defect synapses with 4-bit weigh 1.
	void tagDefectSynapses();

	/// disable defect synapses by setting weight to 0 and decoder to 4-bit address disabling synapse
	void disableDefectSynapes();

	/// Coordinate of HICANN chip we are currently working on.
	HMF::Coordinate::HICANNGlobal const& m_hicann;
	BioGraph const& m_bio_graph;
	hardware_system_t& m_hardware;
	resource_manager_t& m_resource_manager;
	parameters::SynapseRouting const& m_parameters;
	placement::results::Placement const& m_neuron_placement;
	results::L1Routing const& m_l1_routing;
	boost::shared_ptr<SynapseLoss> m_synapse_loss;

	Result m_result;
}; // SynapseRouting

} // namespace routing
} // namespace marocco
