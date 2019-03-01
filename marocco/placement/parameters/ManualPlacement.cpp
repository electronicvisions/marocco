#include "marocco/placement/parameters/ManualPlacement.h"

#include <boost/serialization/map.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/serialization/variant.hpp>
#include <boost/serialization/vector.hpp>

using namespace HMF::Coordinate;

namespace marocco {
namespace placement {
namespace parameters {

void ManualPlacement::on_hicann(
    population_type pop, mask_type mask, HICANNOnWafer const& hicann, size_type size)
{
	on_hicann(pop, mask, std::vector<HICANNOnWafer>{hicann}, size);
}
void ManualPlacement::on_hicann(
    population_type pop, mask_type mask, std::vector<HICANNOnWafer> const& hicanns, size_type size)
{
	if (hicanns.empty()) {
		throw std::invalid_argument("at least one HICANN has to be specified");
	}
	if (size != 0) {
		check_neuron_size(size);
	}
	m_mapping[pop].push_back(Location{hicanns, size, mask});
}

void ManualPlacement::on_neuron_block(
    population_type pop, mask_type mask, NeuronBlockOnWafer const& block, size_type size)
{
	on_neuron_block(pop, mask, std::vector<NeuronBlockOnWafer>{block}, size);
}

void ManualPlacement::on_neuron_block(
    population_type pop,
    mask_type mask,
    std::vector<NeuronBlockOnWafer> const& blocks,
    size_type size)
{
	if (blocks.empty()) {
		throw std::invalid_argument("at least one neuron block has to be specified");
	}
	if (size != 0) {
		check_neuron_size(size);
	}
	m_mapping[pop].push_back(Location{blocks, size, mask});
}

void ManualPlacement::on_neuron(population_type pop, mask_type mask, LogicalNeuron const& neuron)
{
	on_neuron(pop, mask, std::vector<LogicalNeuron>{neuron});
}

void ManualPlacement::on_neuron(
    population_type pop, mask_type mask, std::vector<LogicalNeuron> const& neurons)
{
	if (neurons.empty()) {
		throw std::invalid_argument("at least one neuron has to be specified");
	}
	m_mapping[pop].push_back(Location{neurons, 0, mask});
}

void ManualPlacement::with_size(population_type pop, mask_type mask, size_type size)
{
	if (size == 0u) {
		throw std::invalid_argument("neuron size has to be specified");
	}
	if (size != 0) {
		check_neuron_size(size);
	}
	m_mapping[pop].push_back(Location{std::vector<HICANNOnWafer>{}, size, mask});
}

auto ManualPlacement::mapping() const -> mapping_type const&
{
	return m_mapping;
}

template <typename Archive>
void ManualPlacement::Location::serialize(Archive& ar, unsigned int const /* version */)
{
	using namespace boost::serialization;
	// clang-format off
	ar & make_nvp("locations", locations)
	   & make_nvp("hw_neuron_size", hw_neuron_size)
	   & make_nvp("mask", mask);
	// clang-format on
}

template <typename Archive>
void ManualPlacement::serialize(Archive& ar, unsigned int const /* version */)
{
	using namespace boost::serialization;
	ar & make_nvp("mapping", m_mapping);
}

} // namespace parameters
} // namespace placement
} // namespace marocco

BOOST_CLASS_EXPORT_IMPLEMENT(::marocco::placement::parameters::ManualPlacement)
BOOST_CLASS_EXPORT_IMPLEMENT(::marocco::placement::parameters::ManualPlacement::Location)

#include "boost/serialization/serialization_helper.tcc"
EXPLICIT_INSTANTIATE_BOOST_SERIALIZE(::marocco::placement::parameters::ManualPlacement)
EXPLICIT_INSTANTIATE_BOOST_SERIALIZE(::marocco::placement::parameters::ManualPlacement::Location)
