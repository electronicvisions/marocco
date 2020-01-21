#include "marocco/parameter/AnalogOutputs.h"

#include <type_traits>

#include "euter/typedcellparametervector.h"
#include "halco/common/iter_all.h"

#include "marocco/parameter/detail.h"
#include "marocco/Logger.h"

using namespace halco::hicann::v2;

namespace marocco {
namespace parameter {

namespace {

struct AnalogVisitor
{
	typedef void return_type;
	typedef size_t size_type;

	template <euter::CellType N>
	using cell_t = euter::TypedCellParameterVector<N>;

	// Overload for neurons which support voltage recording
	template <euter::CellType N>
	typename std::enable_if<detail::has_record_v<cell_t<N> >::value, return_type>::type operator()(
	    cell_t<N> const& v,
	    size_t const neuron_index,
	    LogicalNeuron const& logical_neuron,
	    results::AnalogOutputs& result)
	{
		auto const& cell = v.parameters()[neuron_index];
		if (!cell.record_v) {
			return;
		}

		try {
			auto const& item = result.record(logical_neuron);
			MAROCCO_INFO(
				"Using " << item.analog_output() << " on " << item.reticle() << " for "
			    << logical_neuron);
		} catch (ResourceExhaustedError const&) {
			MAROCCO_ERROR("Unable to record " << logical_neuron << ".  No analog outputs left.");
			throw;
		}
	}

	// Overload for neurons which do not support voltage recording
	template <euter::CellType N>
	typename std::enable_if<!detail::has_record_v<cell_t<N> >::value, return_type>::type operator()(
	    cell_t<N> const& /* v */,
	    size_t const /* neuron_index */,
	    LogicalNeuron const& /* neuron */,
	    results::AnalogOutputs& /* result */)
	{
		MAROCCO_WARN("celltype " << (int)N << "doesn't support record_v");
	}
}; // AnalogVisitor

} // namespace

AnalogOutputs::AnalogOutputs(
    BioGraph const& bio_graph, placement::results::Placement const& neuron_placement)
	: m_bio_graph(bio_graph), m_neuron_placement(neuron_placement)
{
}

void AnalogOutputs::run(results::AnalogOutputs& result)
{
	AnalogVisitor visitor;
	auto const& graph = m_bio_graph.graph();
	for (auto const& item : m_neuron_placement) {
		if (item.logical_neuron().is_external()) {
			continue;
		}

		euter::Population const& population = *(graph[item.population()]);
		euter::visitCellParameterVector(
			population.parameters(), visitor, item.neuron_index(), item.logical_neuron(), result);
	}
}

} // namespace parameter
} // namespace marocco
