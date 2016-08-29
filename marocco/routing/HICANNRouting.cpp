#include "marocco/routing/HICANNRouting.h"

#include "marocco/routing/SynapseRouting.h"

using namespace HMF::Coordinate;

namespace marocco {
namespace routing {

HICANNRouting::HICANNRouting(
	BioGraph const& bio_graph,
	hardware_type& hardware,
	resource_manager_t& resource_manager,
	pymarocco::PyMarocco const& pymarocco,
	placement::results::Placement const& neuron_placement,
	results::L1Routing const& l1_routing,
	boost::shared_ptr<SynapseLoss> const& synapse_loss)
	: m_bio_graph(bio_graph),
	  m_hardware(hardware),
	  m_resource_manager(resource_manager),
	  m_pymarocco(pymarocco),
	  m_neuron_placement(neuron_placement),
	  m_l1_routing(l1_routing),
	  m_synapse_loss(synapse_loss)
{
}

void HICANNRouting::run(results::SynapseRouting& result)
{
	for (HICANNGlobal const& hicann : m_resource_manager.allocated()) {
		run(hicann, result);
	}
}

void HICANNRouting::run(
	HMF::Coordinate::HICANNGlobal const& hicann, results::SynapseRouting& result)
{
	if (m_neuron_placement.find(hicann).empty()) {
		// No local neurons, we can skip synapse routing for this HICANN.  This is the
		// case for transit-only HICANNs or HICANNs exclusively used for external input.
		return;
	}

	// synapse routing has to be run no matter there are routes ending at this
	// chip or not, because we need the synapse target mapping for param trafo
	SynapseRouting synapse_routing(
		hicann, m_bio_graph, m_hardware, m_resource_manager, m_pymarocco.synapse_routing,
		m_neuron_placement, m_l1_routing, m_synapse_loss, result);
	synapse_routing.run();
}

} // namespace routing
} // namespace marocco
