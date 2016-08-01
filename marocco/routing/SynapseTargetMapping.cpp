#include "marocco/routing/SynapseTargetMapping.h"

#include <iomanip>

#include "hal/Coordinate/iter_all.h"
#include "marocco/placement/Result.h"
#include "marocco/routing/SynapseTargetVisitor.h"
#include "marocco/util/chunked.h"


using namespace HMF::Coordinate;
using namespace HMF;

namespace marocco {
namespace routing {

SynapseTargetMapping::SynapseTargetMapping()
{
	// initialize all synapse targets to SynapseType::None
	for (auto const& noh : iter_all<NeuronOnHICANN>())
		for (auto const& side : iter_all<SideHorizontal>())
			(*this)[noh][side] = SynapseType::None;
}

/**
 * maps required synapse targets in a simple manner onto connected hardware neurons.
 *
 * This is the actual implementation of SynapseTargetMapping::simple_mapping()
 * for one compound neuron. See there for details.
 *
 * @param[in] required_targets required synapse targets
 * @param[in] neurons hardware neurons of the compound neuron.
 * @param[out] target_mapping synapse target mapping filled by this function
 */
void map_targets(
	std::vector<SynapseType> const& required_targets,
	std::vector<NeuronOnHICANN> const& neurons,
	SynapseTargetMapping& target_mapping)
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
	if (required_targets.size() > top_neurons.size() * HICANN::RowConfig::num_syn_ins) {
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
		// odd columns
		if (nrn.x() % 2) {
			target_mapping[nrn][left] = target_odd_left;
			target_mapping[nrn][right] = target_odd_right;
		}
		// even columns
		else {
			target_mapping[nrn][left] = target_even_left;
			target_mapping[nrn][right] = target_even_right;
		}
	}
}

void SynapseTargetMapping::simple_mapping(
    HMF::Coordinate::HICANNOnWafer const& hicann,
    placement::results::Placement const& neuron_placement,
    graph_t const& graph)
{
	SynapseTargetVisitor const syn_tgt_visitor{};

	for (auto const& item : neuron_placement.find(hicann)) {
		Population const& pop = *(graph[item.population()]);
		auto const& logical_neuron = item.logical_neuron();
		assert(logical_neuron.is_rectangular());

		std::vector<SynapseType> synapse_targets = visitCellParameterVector(
			pop.parameters(), syn_tgt_visitor, item.neuron_index());

		{
			assert(logical_neuron.size() % NeuronOnNeuronBlock::y_type::size == 0);
			size_t const neuron_width =
				logical_neuron.size() / NeuronOnNeuronBlock::y_type::size;
			if (synapse_targets.size() > neuron_width * HICANN::RowConfig::num_syn_ins) {
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

		map_targets(synapse_targets, connected_neurons, *this);
	}
}


bool SynapseTargetMapping::check_top_and_bottom_are_equal() const
{
	bool good = true;
	for (size_t xx = 0; xx < NeuronOnHICANN::x_type::end; ++xx) {
		const NeuronOnHICANN nt(X(xx), top);
		const NeuronOnHICANN nb(X(xx), bottom);
		good &= ((*this)[nt][left] == (*this)[nb][left]);
		good &= ((*this)[nt][right] == (*this)[nb][right]);
	}
	return good;
}

std::ostream& operator<<(std::ostream& os, SynapseTargetMapping const& target_mapping)
{
	for (auto vside : {top, bottom}) {
		std::stringstream row1, row2;
		for (size_t xx = 0; xx < NeuronOnHICANN::x_type::end; ++xx) {
			const NeuronOnHICANN nn(X(xx), vside);
			// synaptic target stuff.
			row1 << "|";
			SynapseType target_left = target_mapping[nn][left];
			// print first character of string equivalent
			// i.e.: 'e' -> excitatory, 'i' -> inhibitory, '0' -> target 0 etc.
			if (target_left != SynapseType::None) {
				std::stringstream ss;
				ss << target_left;
				row1 << ss.str()[0];
			} else {
				row1 << " ";
			}

			row1 << " ";

			SynapseType target_right = target_mapping[nn][right];
			if (target_right != SynapseType::None) {
				std::stringstream ss;
				ss << target_right;
				row1 << ss.str()[0];
			} else {
				row1 << " ";
			}

			// denmems
			row2 << "|" << std::setw(3) << std::setfill(' ') << (size_t)nn.id();
		}
		row1 << "|\n";
		row2 << "|\n";
		if (vside == top)
			os << row1.str() << row2.str();
		else
			os << row2.str() << row1.str();
	}
	return os;
}

} // namespace routing
} // namespace marocco
