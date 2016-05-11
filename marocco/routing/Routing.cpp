#include "marocco/routing/Routing.h"

#include <boost/make_shared.hpp>

#include "marocco/Logger.h"
#include "marocco/routing/Configuration.h"
#include "marocco/routing/HICANNRouting.h"
#include "marocco/routing/HandleSynapseLoss.h"
#include "marocco/routing/L1Routing.h"
#include "marocco/routing/SynapseLoss.h"
#include "marocco/util/guess_wafer.h"

#include "tools/roqt/bindings/pyroqt.h"

namespace marocco {
namespace routing {

Routing::Routing(
	BioGraph const& graph,
	hardware_system_t& hardware,
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

std::unique_ptr<Result> Routing::run(results::L1Routing& l1_routing_result)
{
	std::unique_ptr<Result> result(new Result);

	m_synapse_loss = boost::make_shared<SynapseLoss>(m_graph.graph());

	{
		L1RoutingGraph l1_graph;

		MAROCCO_INFO("Setting up L1 routing graph");
		for (auto const& hicann : m_resource_manager.present()) {
			l1_graph.add(hicann);
		}

		// We need to deal with defects in a separate step since each call to
		// `l1_graph.add` adds new edges that may need to be removed because of defects.
		for (auto const& hicann : m_resource_manager.present()) {
			auto const defects = m_resource_manager.get(hicann);
			auto const hbuses = defects->hbuses();
			for (auto it = hbuses->begin_disabled(); it != hbuses->end_disabled(); ++it) {
				l1_graph.remove(hicann, *it);
				MAROCCO_DEBUG("Marked " << *it << " on " << hicann << " as defect/disabled");
			}

			auto const vbuses = defects->vbuses();
			for (auto it = vbuses->begin_disabled(); it != vbuses->end_disabled(); ++it) {
				l1_graph.remove(hicann, *it);
				MAROCCO_DEBUG("Marked " << *it << " on " << hicann << " as defect/disabled");
			}
		}

		MAROCCO_INFO("Beginning L1 routing");
		L1Routing l1_routing(
		    l1_graph, m_graph, m_pymarocco.l1_routing, m_neuron_placement, l1_routing_result);
		l1_routing.run();
		MAROCCO_INFO("L1 routing finished with " << l1_routing_result.size() << " routes");

		auto const& failed = l1_routing.failed_routes();
		if (!failed.empty()) {
			MAROCCO_WARN("Failed to establish " << failed.size() << " routes");
		}

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
	auto& wafer = m_hardware[guess_wafer(m_resource_manager)];
	for (auto const& item : l1_routing_result) {
		configure(wafer, item.route());
	}

	HICANNRouting local_router(
		m_graph, m_hardware, m_resource_manager, m_pymarocco, m_neuron_placement,
		l1_routing_result, m_synapse_loss);
	result->synapse_routing = local_router.run();

	// Remove PyRoQt as soon as all results are integrated into marocco_results.
	if (!m_pymarocco.roqt.empty()) {
		PyRoQt pyroqt(l1_routing_result, result->synapse_routing);
		pyroqt.store(m_pymarocco.roqt);
	}

	return result;
}

boost::shared_ptr<SynapseLoss> Routing::getSynapseLoss() const
{
	return m_synapse_loss;
}

} // namespace routing
} // namespace marocco
