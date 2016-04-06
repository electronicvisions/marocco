#include "marocco/placement/Placement.h"

#include "marocco/Logger.h"
#include "marocco/placement/InputPlacement.h"
#include "marocco/placement/LookupTable.h"
#include "marocco/placement/MergerRouting.h"
#include "marocco/placement/MergerTreeConfigurator.h"
#include "marocco/placement/NeuronPlacement.h"

using namespace HMF::Coordinate;

namespace marocco {
namespace placement {

namespace {

template <typename T>
void handle_defects(MergerTreeGraph& graph, HICANNOnWafer const& hicann, T const& res)
{
	for (auto it = res->begin_disabled(); it != res->end_disabled(); ++it) {
		graph.remove(*it);
		debug(&graph) << "Marked " << *it << " on " << hicann << " as defect/disabled";
	}
}

void handle_defects(
    MergerTreeGraph& graph, HICANNOnWafer const& hicann, redman::resources::Hicann const& defects)
{
	handle_defects(graph, hicann, defects.mergers0());
	handle_defects(graph, hicann, defects.mergers1());
	handle_defects(graph, hicann, defects.mergers2());
	handle_defects(graph, hicann, defects.mergers3());
	handle_defects(graph, hicann, defects.dncmergers());
}

} // namespace

Placement::Placement(
    pymarocco::PyMarocco& pymarocco,
    graph_t const& graph,
    hardware_system_t& hardware,
    resource_manager_t& resource_manager)
    : m_graph(graph),
      m_hardware(hardware),
      m_resource_manager(resource_manager),
      m_pymarocco(pymarocco)
{
}

auto Placement::run() -> std::unique_ptr<result_type>
{
	std::unique_ptr<Result> result(new Result);

	auto const wafers = m_resource_manager.wafers();
	BOOST_ASSERT_MSG(wafers.size() == 1, "only single-wafer use is supported");

	NeuronPlacement nrn_placement(
	    m_graph, m_pymarocco.neuron_placement, m_pymarocco.manual_placement,
	    result->neuron_placement);

	for (auto const& hicann : m_resource_manager.present()) {
		nrn_placement.add(hicann);
		auto const& neurons = m_resource_manager.get(hicann)->neurons();
		for (auto it = neurons->begin_disabled(); it != neurons->end_disabled(); ++it) {
			nrn_placement.add_defect(hicann, *it);
		}
	}

	nrn_placement.run();

	MergerRouting merger_routing(
	    m_pymarocco.merger_routing, result->neuron_placement, result->merger_routing);

	for (auto const& item : result->neuron_placement.denmem_assignment()) {
		// Tag HICANN as 'in use' in the resource manager.
		HICANNGlobal hicann(item.first, wafers.front());
		if (m_resource_manager.available(hicann)) {
			m_resource_manager.allocate(hicann);
		}

		// Set up merger tree graph and remove defect mergers.
		MergerTreeGraph merger_graph;
		MAROCCO_DEBUG("Disabling defect mergers on " << hicann);
		handle_defects(merger_graph, hicann, *(m_resource_manager.get(hicann)));

		// Run merger routing.
		merger_routing.run(merger_graph, hicann);
		auto const& merger_mapping = result->merger_routing[hicann];

		// Apply merger tree configuration to sthal container.
		{
			auto& chip = m_hardware[hicann];

			MergerTreeConfigurator configurator(chip.layer1, merger_graph, merger_mapping);
			configurator.run();
		}

		// Assign and store L1 addresses.
		auto& address_assignment = result->address_assignment[hicann];
		for (auto const& it : merger_mapping)
		{
			NeuronBlockOnWafer const neuron_block(it.first, hicann);
			DNCMergerOnWafer const dnc(it.second, hicann);

			// set this SPL1 merger to output
			address_assignment.set_mode(dnc, L1AddressAssignment::Mode::output);
			auto& pool = address_assignment.available_addresses(dnc);

			for (auto const& item : result->neuron_placement.find(neuron_block)) {
				auto const address = pool.pop(m_pymarocco.l1_address_assignment.strategy());
				auto const& logical_neuron = item.logical_neuron();
				result->neuron_placement.set_address(
				    logical_neuron, L1AddressOnWafer(dnc, address));
			}
		}
	}

	// placement of externals, eg spike inputs
	InputPlacement input_placement(
	    m_graph, m_pymarocco.input_placement, m_pymarocco.manual_placement,
	    m_pymarocco.neuron_placement, m_pymarocco.l1_address_assignment, m_pymarocco.speedup,
	    m_hardware, m_resource_manager);
	input_placement.run(result->neuron_placement, result->address_assignment);

	// create reverse mapping
	result->reverse_mapping = std::make_shared<LookupTable>(*result, m_resource_manager, m_graph);

	return { std::move(result) };
}

} // namespace placement
} // namespace marocco
