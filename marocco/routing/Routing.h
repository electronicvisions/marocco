#pragma once

#include <boost/shared_ptr.hpp>

#include "marocco/BioGraph.h"
#include "marocco/config.h"
#include "marocco/placement/results/Placement.h"
#include "marocco/routing/SynapseLoss.h"
#include "marocco/routing/results/L1Routing.h"
#include "marocco/routing/results/SynapseRouting.h"
#include "pymarocco/PyMarocco.h"

namespace marocco {
namespace routing {

class Routing
{
public:
	Routing(
		BioGraph const& graph,
		hardware_system_t& hardware,
		resource_manager_t& resource_manager,
		pymarocco::PyMarocco& pymarocco,
		placement::results::Placement const& neuron_placement);

	void run(
	    results::L1Routing& l1_routing_result, results::SynapseRouting& synapse_routing_result);
	boost::shared_ptr<SynapseLoss> getSynapseLoss() const;

private:
	BioGraph const& m_graph;
	hardware_system_t& m_hardware;
	resource_manager_t& m_resource_manager;
	pymarocco::PyMarocco& m_pymarocco;
	placement::results::Placement const& m_neuron_placement;
	boost::shared_ptr<SynapseLoss> m_synapse_loss;
};

} // namespace routing
} // namespace marocco
