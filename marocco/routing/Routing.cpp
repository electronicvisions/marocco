#include <chrono>
#include <iterator>

#include "marocco/routing/Routing.h"

#include <boost/make_shared.hpp>

#include "marocco/Logger.h"
#include "marocco/routing/Configuration.h"
#include "marocco/routing/HICANNRouting.h"
#include "marocco/routing/HandleSynapseLoss.h"
#include "marocco/routing/L1Routing.h"
#include "marocco/routing/SynapseLoss.h"
#include "marocco/routing/SynapseRoutingConfigurator.h"

#include <boost/make_shared.hpp>

using namespace halco::hicann::v2;

namespace marocco {
namespace routing {

Routing::Routing(
	BioGraph const& graph,
	sthal::Wafer& hardware,
	resource_manager_t& resource_manager,
	pymarocco::PyMarocco& pymarocco,
	placement::results::Placement const& neuron_placement)
	: m_graph(graph),
	  m_hardware(hardware),
	  m_resource_manager(resource_manager),
	  m_pymarocco(pymarocco),
	  m_neuron_placement(neuron_placement)
{
}

void Routing::run(results::L1Routing& l1_routing_result, results::SynapseRouting& synapse_routing_result)
{
	m_synapse_loss = boost::make_shared<SynapseLoss>(m_graph.graph());

	{
		L1RoutingGraph l1_graph(m_pymarocco.l1_routing);

		MAROCCO_INFO("Setting up L1 routing graph");
		for (auto const& hicann : m_resource_manager.present()) {
			l1_graph.add(hicann);
		}

		// We need to deal with defects in a separate step since each call to
		// `l1_graph.add` adds new edges that may need to be removed because of defects.
		for (auto const& hicann : m_resource_manager.present()) {
			auto const defects = m_resource_manager.get(hicann);

			// horizontal buses and repeaters
			auto const hbuses = defects->hbuses();
			for (auto const& hb : hbuses->disabled()) {
				l1_graph.remove(hicann, hb);
				MAROCCO_TRACE("Marked " << hb << " on " << hicann << " as defect/disabled");
			}

			size_t const n_marked_hbuses = boost::size(hbuses->disabled());
			if (n_marked_hbuses != 0) {
				MAROCCO_DEBUG("Marked " << n_marked_hbuses << " horizontal L1 bus(es) on "
				                        << hicann << " as defect/disabled");
			}

			auto const hrepeaters = defects->hrepeaters();
			for (auto const& hr : hrepeaters->disabled()) {
				l1_graph.remove(hicann, hr);
				MAROCCO_TRACE("Marked " << hr << " on " << hicann << " as defect/disabled");
			}

			size_t const n_marked_hrepeaters = boost::size(hrepeaters->disabled());
			if (n_marked_hrepeaters != 0) {
				MAROCCO_DEBUG("Marked " << n_marked_hrepeaters << " horizontal L1 repeater(s) on "
				                        << hicann << " as defect/disabled");
			}

			// vertical buses and repeaters
			auto const vbuses = defects->vbuses();
			for (auto const& vb : vbuses->disabled()) {
				l1_graph.remove(hicann, vb);
				MAROCCO_TRACE("Marked " << vb << " on " << hicann << " as defect/disabled");
			}

			size_t const n_marked_vbuses = boost::size(vbuses->disabled());
			if (n_marked_vbuses != 0) {
				MAROCCO_DEBUG("Marked " << n_marked_vbuses << " vertical L1 bus(es) on "
				                        << hicann << " as defect/disabled");
			}

			auto const vrepeaters = defects->vrepeaters();
			for (auto const& vr : vrepeaters->disabled()) {
				l1_graph.remove(hicann, vr);
				MAROCCO_TRACE("Marked " << vr << " on " << hicann << " as defect/disabled");
			}

			size_t const n_marked_vrepeaters = boost::size(vrepeaters->disabled());
			if (n_marked_vrepeaters != 0) {
				MAROCCO_DEBUG("Marked " << n_marked_vrepeaters << " vertical L1 repeater(s) on "
				                        << hicann << " as defect/disabled");
			}

		}

		auto startL1Routing = std::chrono::system_clock::now();
		L1Routing l1_routing(
		    l1_graph, m_graph, m_pymarocco.l1_routing, m_neuron_placement, l1_routing_result,
		    m_resource_manager);
		l1_routing.run();
		MAROCCO_INFO("L1 routing finished with " << l1_routing_result.size() << " routes");

		auto const& failed = l1_routing.failed_routes();
		if (!failed.empty()) {
			MAROCCO_WARN("Failed to establish " << failed.size() << " routes");
		}
		auto endL1Routing = std::chrono::system_clock::now();
		m_pymarocco.stats.timeL1Routing =
		    std::chrono::duration_cast<std::chrono::milliseconds>(endL1Routing - startL1Routing)
		        .count();

		// Track synapse loss
		HandleSynapseLoss handle_synapse_loss(m_graph, m_neuron_placement, l1_routing_result, m_synapse_loss);
		for (auto const& route : failed) {
			for (auto const& edge : route.projections) {
				handle_synapse_loss(route.source, route.target, edge);
			}
		}

		size_t const synapse_loss = m_synapse_loss->getTotalLoss();
		m_pymarocco.stats.setSynapseLossAfterL1Routing(synapse_loss);
	}

	MAROCCO_INFO("Configuring L1 routes");
	auto& wafer_config = m_hardware;
	for (auto const& item : l1_routing_result) {
		configure(wafer_config, item.route());
	}

	auto startSynRouting = std::chrono::system_clock::now();
	HICANNRouting local_router(
		m_graph, wafer_config, m_resource_manager, m_pymarocco, m_neuron_placement,
		l1_routing_result, m_synapse_loss);
	local_router.run(synapse_routing_result);
	auto endSynRouting = std::chrono::system_clock::now();
	m_pymarocco.stats.timeSynRouting =
	    std::chrono::duration_cast<std::chrono::milliseconds>(endSynRouting - startSynRouting)
	        .count();

	SynapseRoutingConfigurator configurator(wafer_config);

	for (auto const& hicann : m_resource_manager.allocated()) {
		if (!synapse_routing_result.has(hicann)) {
			MAROCCO_DEBUG(
			    "no synapse routing result for " << hicann
			    << "\nThis is ok, if there are no neurons placed on this HICANN or no routes "
			    "could be realized to connect to neurons on this chip");
			continue;
		}

		configurator.run(hicann, synapse_routing_result[hicann]);
	}

	// Allocate all HICANNs used in L1 routes s.t. shared parameters will be configured later.
	for (auto const& item : l1_routing_result) {
		for (auto const& segment : item.route().segments()) {
			if (auto const* hicann = boost::get<HICANNOnWafer>(&segment)) {
				HICANNGlobal const resource(*hicann, m_hardware.index());
				if (m_resource_manager.available(resource)) {
					m_resource_manager.allocate(resource);
				}
			}
		}
	}
}

boost::shared_ptr<SynapseLoss> Routing::getSynapseLoss() const
{
	return m_synapse_loss;
}

} // namespace routing
} // namespace marocco
