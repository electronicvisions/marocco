#pragma once

#include "sthal/Wafer.h"
#include "marocco/config.h"

#include "marocco/BioGraph.h"
#include "marocco/placement/results/Placement.h"
#include "marocco/routing/L1RoutingGraph.h"
#include "marocco/routing/SynapseRowSource.h"
#include "marocco/routing/results/SynapseRouting.h"
#include "marocco/routing/results/L1Routing.h"
#include "pymarocco/PyMarocco.h"

namespace marocco {
namespace routing {

class SynapseLoss;

class HICANNRouting
{
public:
	typedef sthal::Wafer hardware_type;

	HICANNRouting(
		BioGraph const& bio_graph,
		hardware_type& hardware,
		resource_manager_t& resource_manager,
		pymarocco::PyMarocco const& pymarocco,
		placement::results::Placement const& neuron_placement,
		results::L1Routing const& l1_routing,
		boost::shared_ptr<SynapseLoss> const& synapse_loss);

	void run(results::SynapseRouting& result);

private:
	void run(halco::hicann::v2::HICANNGlobal const& hicann, results::SynapseRouting& result);

	BioGraph const& m_bio_graph;
	hardware_type& m_hardware;
	resource_manager_t& m_resource_manager;
	pymarocco::PyMarocco const& m_pymarocco;
	placement::results::Placement const& m_neuron_placement;
	results::L1Routing const& m_l1_routing;
	boost::shared_ptr<SynapseLoss> m_synapse_loss;
};

} // namespace routing
} // namespace marocco
