#include "marocco/routing/results/SynapticInputs.h"

#include <iostream>
#include <iomanip>
#include <string>

#include "hal/Coordinate/Relations.h"
#include "hal/Coordinate/iter_all.h"

using namespace HMF::Coordinate;

namespace marocco {
namespace routing {
namespace results {

SynapticInputs::SynapticInputs()
{
	value_type inputs;
	for (auto const& syn_input : iter_all<SynapticInputOnNeuron>()) {
		inputs[syn_input] = SynapseType::none;
	}

	for (auto const& neuron : iter_all<NeuronOnHICANN>()) {
		m_neurons[neuron] = inputs;
	}
}

auto SynapticInputs::operator[](HMF::Coordinate::NeuronOnHICANN const& neuron) -> value_type&
{
	return m_neurons[neuron];
}

auto SynapticInputs::operator[](HMF::Coordinate::NeuronOnHICANN const& neuron) const
	-> value_type const&
{
	return m_neurons[neuron];
}

bool SynapticInputs::is_horizontally_symmetrical() const
{
	for (auto xx : iter_all<NeuronOnHICANN::x_type>()) {
		NeuronOnHICANN const neuron_top(xx, top);
		NeuronOnHICANN const neuron_bottom(xx, bottom);
		if ((m_neurons[neuron_top][left] != m_neurons[neuron_bottom][left]) ||
		    (m_neurons[neuron_top][right] != m_neurons[neuron_bottom][right])) {
			return false;
		}
	}
	return true;
}

template <typename Archiver>
void SynapticInputs::serialize(Archiver& ar, unsigned int const /*version*/)
{
	using namespace boost::serialization;
	ar & make_nvp("neurons", m_neurons);
}

std::ostream& operator<<(std::ostream& os, SynapticInputs const& synaptic_inputs)
{
	std::string const horizontal_line(NeuronOnNeuronBlock::x_type::size * 4 + 9, '-');
	os << horizontal_line << "\n";

	auto print_target = [&os](SynapseType const& target) {
		int value = int(target);
		if (value >= 0) {
			os << value;
			return;
		}
		os << ((target == SynapseType::excitatory)
		       ? "e" : ((target == SynapseType::inhibitory) ? "i" : " "));

	};

	// Print mapping by Neuron blocks
	for (auto const nb : iter_all<NeuronBlockOnHICANN>()) {
		os << "|  NB(" << size_t(nb) << ") ";
		for (auto xx : iter_all<NeuronOnNeuronBlock::x_type>()) {
			os << "|" << std::setw(3) << std::setfill(' ') << size_t(xx);
		}
		os << "|\n";

		// For each neuron print the first character of synapse type, i.e.:
		// 'e' -> excitatory, 'i' -> inhibitory, '0' -> target 0 etc.
		for (auto yy : iter_all<NeuronOnNeuronBlock::y_type>()) {
			os << "| " << (yy == top ? "   top" : "bottom") << " ";

			for (auto xx : iter_all<NeuronOnNeuronBlock::x_type>()) {
				auto const nrn = NeuronOnNeuronBlock(xx, yy).toNeuronOnHICANN(nb);
				os << "|";
				print_target(synaptic_inputs[nrn][left]);
				os << " ";
				print_target(synaptic_inputs[nrn][right]);
			}
			os << "|\n";
		}
		os << horizontal_line << "\n";
	}
	return os;
}

} // namespace results
} // namespace routing
} // namespace marocco

BOOST_CLASS_EXPORT_IMPLEMENT(::marocco::routing::results::SynapticInputs)

#include "boost/serialization/serialization_helper.tcc"
EXPLICIT_INSTANTIATE_BOOST_SERIALIZE(::marocco::routing::results::SynapticInputs)
