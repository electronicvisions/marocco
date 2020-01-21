#include "marocco/routing/internal/SynapseTargetMapping.h"

#include "halco/common/relations.h"
#include "halco/common/iter_all.h"
#include "marocco/routing/internal/SynapseTargetVisitor.h"

using namespace halco::hicann::v2;
using namespace halco::common;

namespace marocco {
namespace routing {
namespace internal {

namespace {

/**
 * maps required synapse targets in a simple manner onto connected hardware neurons.
 *
 * This is the actual implementation of SynapseTargetMapping::simple_mapping()
 * for one compound neuron. See there for details.
 *
 * @param[in] required_targets required synapse targets
 * @param[in] neurons hardware neurons of the compound neuron.
 * @param[out] synaptic_inputs synapse target mapping filled by this function
 */
void map_targets(
	std::vector<SynapseType> const& required_targets,
	std::vector<NeuronOnHICANN> const& neurons,
	results::SynapticInputs& synaptic_inputs)
{
	if (required_targets.size() == 0)
		return;

	std::set<NeuronOnHICANN> top_neurons, bot_neurons;
	for (auto const& nrn : neurons) {
		if (nrn.y() == top)
			top_neurons.insert(nrn);
		else
			bot_neurons.insert(nrn);
	}

	// check that there are as many neurons in top and bottom block,
	// that they have the same x-range,
	// and have no gaps
	if (top_neurons.size() != bot_neurons.size() ||
		top_neurons.cbegin()->x() != bot_neurons.cbegin()->x() ||
		top_neurons.crbegin()->x() != bot_neurons.crbegin()->x() ||
		top_neurons.crbegin()->x() - top_neurons.cbegin()->x() != top_neurons.size() - 1) {
		throw std::runtime_error(
			"map_targets: passed neuron coordinates don't build a connected block of neurons");
	}

	// TODO: Should we check if it was already assigned, i.e. str == ""?
	if (required_targets.size() > top_neurons.size() * SynapticInputOnNeuron::size) {
		throw std::runtime_error(
			"Neuron has more synaptic time constants than provided by placement. \
					HardwareNeuronSize should be >= nr of synaptic time constants.");
	}

	SynapseType target_even_left;
	SynapseType target_even_right;
	SynapseType target_odd_left;
	SynapseType target_odd_right;

	// avoid that a target is mapped on different (side) settings, to simplify routing afterwards
	// FOR NOW, we only support up to 4 time constants, to simplify the stuff.

	switch (required_targets.size()) {
		case 1:
			target_even_left = required_targets[0];
			target_even_right = required_targets[0];
			target_odd_left = required_targets[0];
			target_odd_right = required_targets[0];
			break;

		case 2:
			target_even_left = required_targets[0];
			target_even_right = required_targets[1];
			target_odd_left = required_targets[0];
			target_odd_right = required_targets[1];
			break;

		case 3:
			// target[0] gets two inputs, both on the left side.
			target_even_left = required_targets[0];
			target_even_right = required_targets[1];
			target_odd_left = required_targets[0];
			target_odd_right = required_targets[2];
			break;

		case 4:
			target_even_left = required_targets[0];
			target_even_right = required_targets[1];
			target_odd_left = required_targets[2];
			target_odd_right = required_targets[3];
			break;

		default:
			throw std::runtime_error(
				"mapping of more than 4 synaptic targets (i.e. time constants) not yet supported");
			break;
	}

	for (auto const& nrn : neurons) {
		if (nrn.x() % 2) {
			// odd columns
			synaptic_inputs[nrn][left] = target_odd_left;
			synaptic_inputs[nrn][right] = target_odd_right;
		} else {
			// even columns
			synaptic_inputs[nrn][left] = target_even_left;
			synaptic_inputs[nrn][right] = target_even_right;
		}
	}
}

} // namespace

void SynapseTargetMapping::simple_mapping(
	halco::hicann::v2::HICANNOnWafer const& hicann,
	placement::results::Placement const& neuron_placement,
	graph_t const& graph,
	results::SynapticInputs& synaptic_inputs)
{
	SynapseTargetVisitor const syn_tgt_visitor{};

	for (auto const& item : neuron_placement.find(hicann)) {
		euter::Population const& pop = *(graph[item.population()]);
		std::vector<SynapseType> synapse_targets = visitCellParameterVector(
			pop.parameters(), syn_tgt_visitor, item.neuron_index());

		auto const& logical_neuron = item.logical_neuron();
		// Assumes rectangular neuron shapes spanning both rows.
		assert(logical_neuron.is_rectangular());

		{
			assert(logical_neuron.size() % NeuronOnNeuronBlock::y_type::size == 0);
			size_t const neuron_width =
				logical_neuron.size() / NeuronOnNeuronBlock::y_type::size;
			if (synapse_targets.size() > neuron_width * SynapticInputOnNeuron::size) {
				throw std::runtime_error(
					"Neuron has more synaptic time constants than provided by placement. "
					"HardwareNeuronSize should be >= # of synaptic time constants.");
			}
		}

		std::vector<NeuronOnHICANN> connected_neurons;
		connected_neurons.reserve(logical_neuron.size());
		for (NeuronOnHICANN nrn : logical_neuron) {
			connected_neurons.push_back(nrn);
		}

		map_targets(synapse_targets, connected_neurons, synaptic_inputs);
	}
}

} // namespace internal
} // namespace routing
} // namespace marocco
