#pragma once

#include "halco/hicann/v2/hicann.h"
#include "halco/hicann/v2/l1.h"
#include "sthal/Wafer.h"

#include "marocco/BioGraph.h"
#include "marocco/config.h"
#include "marocco/placement/results/Placement.h"
#include "marocco/routing/L1RoutingGraph.h"
#include "marocco/routing/SynapseRowSource.h"
#include "marocco/routing/parameters/SynapseRouting.h"
#include "marocco/routing/results/L1Routing.h"
#include "marocco/routing/results/SynapseRouting.h"

namespace marocco {
namespace routing {

class SynapseLoss;

class SynapseRouting
{
public:
	typedef sthal::Wafer hardware_type;

	SynapseRouting(
		halco::hicann::v2::HICANNGlobal const& hicann,
		BioGraph const& bio_graph,
		hardware_type& hardware,
		resource_manager_t& resource_manager,
		parameters::SynapseRouting const& parameters,
		placement::results::Placement const& neuron_placement,
		results::L1Routing const& l1_routing,
		boost::shared_ptr<SynapseLoss> const& synapse_loss,
		results::SynapseRouting& result);

	void run();

private:
	void handleSynapseLoss(results::L1Routing::route_item_type const& route_item);

	/// set all synapse decoder to the the 4-bit address disabling the synapse.
	void invalidateSynapseDecoder();

	/// temporariliy tag defect synapses with 4-bit weigh 1.
	void tagDefectSynapses();

	/// disable defect synapses by setting weight to 0 and decoder to 4-bit address disabling synapse
	void disableDefectSynapes();

	/// Coordinate of HICANN chip we are currently working on.
	halco::hicann::v2::HICANNGlobal const& m_hicann;
	BioGraph const& m_bio_graph;
	hardware_type& m_hardware;
	resource_manager_t& m_resource_manager;
	parameters::SynapseRouting const& m_parameters;
	placement::results::Placement const& m_neuron_placement;
	results::L1Routing const& m_l1_routing;
	boost::shared_ptr<SynapseLoss> m_synapse_loss;

	results::SynapseRouting& m_result;
}; // SynapseRouting

} // namespace routing
} // namespace marocco
