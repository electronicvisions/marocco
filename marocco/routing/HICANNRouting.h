#pragma once

#include "marocco/config.h"

#include "marocco/BioGraph.h"
#include "marocco/placement/results/Placement.h"
#include "marocco/routing/L1RoutingGraph.h"
#include "marocco/routing/Result.h"
#include "marocco/routing/results/L1Routing.h"
#include "pymarocco/PyMarocco.h"

namespace marocco {
namespace routing {

class SynapseLoss;

class HICANNRouting
{
public:
	typedef hardware_system_t hardware_type;
	typedef chip_type<hardware_type>::type chip_type;

	HICANNRouting(
		BioGraph const& bio_graph,
		hardware_system_t& hardware,
		resource_manager_t& resource_manager,
		pymarocco::PyMarocco const& pymarocco,
		placement::results::Placement const& neuron_placement,
		results::L1Routing const& l1_routing,
		boost::shared_ptr<SynapseLoss> const& synapse_loss);

	SynapseRoutingResult run();

private:
	void run(HMF::Coordinate::HICANNGlobal const& hicann, SynapseRoutingResult& result);

	void configure_hardware(routing::SynapseRoutingResult const& syndrvrouting);

	void setSynapseSwitch(
		HMF::Coordinate::VLineOnHICANN const& vline,
		HMF::Coordinate::SynapseSwitchRowOnHICANN const& row,
		chip_type& chip);

	void setSynapseDriver(
		DriverResult const& d,
		chip_type& chip);

	template<typename RAIter>
	void connectSynapseDriver(
		HMF::Coordinate::SynapseDriverOnHICANN const& primary,
		RAIter const first,
		RAIter const last,
		chip_type& chip);

	BioGraph const& m_bio_graph;
	hardware_system_t& m_hardware;
	resource_manager_t& m_resource_manager;
	pymarocco::PyMarocco const& m_pymarocco;
	placement::results::Placement const& m_neuron_placement;
	results::L1Routing const& m_l1_routing;
	boost::shared_ptr<SynapseLoss> m_synapse_loss;
};

} // namespace routing
} // namespace marocco
