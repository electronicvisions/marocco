#include "marocco/placement/parameters/ManualPlacement.h"

#include <boost/serialization/map.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/serialization/variant.hpp>
#include <boost/serialization/vector.hpp>

using namespace HMF::Coordinate;

namespace marocco {
namespace placement {
namespace parameters {

void ManualPlacement::on_hicann(population_type pop, HICANNOnWafer const& hicann, size_type size)
{
	on_hicann(pop, std::vector<HICANNOnWafer>{hicann}, size);
}

void ManualPlacement::on_hicann(
    population_type pop, std::vector<HICANNOnWafer> const& hicanns, size_type size)
{
	check_neuron_size(size);
	m_mapping[pop] = Location{hicanns, size};
}

void ManualPlacement::on_neuron_block(
    population_type pop, NeuronBlockOnWafer const& block, size_type size)
{
	on_neuron_block(pop, std::vector<NeuronBlockOnWafer>{block}, size);
}

void ManualPlacement::on_neuron_block(
    population_type pop, std::vector<NeuronBlockOnWafer> const& blocks, size_type size)
{
	check_neuron_size(size);
	m_mapping[pop] = Location{blocks, size};
}

void ManualPlacement::with_size(population_type pop, size_type size)
{
	if (size == 0u) {
		throw std::invalid_argument("neuron size has to be specified");
	}
	check_neuron_size(size);
	m_mapping[pop] = Location{std::vector<HICANNOnWafer>{}, size};
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
	   & make_nvp("hw_neuron_size", hw_neuron_size);
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
