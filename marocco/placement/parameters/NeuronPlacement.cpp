#include "marocco/placement/parameters/NeuronPlacement.h"

#include <boost/serialization/nvp.hpp>

#include "hal/Coordinate/Neuron.h"
#include "marocco/coordinates/LogicalNeuron.h"

namespace marocco {
namespace placement {
namespace parameters {

NeuronPlacement::NeuronPlacement()
	: m_default_neuron_size(4),
	  m_restrict_rightmost_neuron_blocks(false),
	  m_minimize_number_of_sending_repeaters(false),
	  m_skip_hicanns_without_neuron_blacklisting(true)
{
}

void NeuronPlacement::default_neuron_size(size_type size)
{
	check_neuron_size(size);
	m_default_neuron_size = size;
}

auto NeuronPlacement::default_neuron_size() const -> size_type
{
	return m_default_neuron_size;
}

void NeuronPlacement::restrict_rightmost_neuron_blocks(bool enable)
{
	m_restrict_rightmost_neuron_blocks = enable;
}

bool NeuronPlacement::restrict_rightmost_neuron_blocks() const
{
	return m_restrict_rightmost_neuron_blocks;
}

void NeuronPlacement::minimize_number_of_sending_repeaters(bool enable)
{
	m_minimize_number_of_sending_repeaters = enable;
}

bool NeuronPlacement::minimize_number_of_sending_repeaters() const
{
	return m_minimize_number_of_sending_repeaters;
}

void NeuronPlacement::skip_hicanns_without_neuron_blacklisting(bool enable)
{
	m_skip_hicanns_without_neuron_blacklisting = enable;
}

bool NeuronPlacement::skip_hicanns_without_neuron_blacklisting() const
{
	return m_skip_hicanns_without_neuron_blacklisting;
}

template <typename Archive>
void NeuronPlacement::serialize(Archive& ar, unsigned int const /* version */)
{
	using namespace boost::serialization;
	// clang-format off
	ar & make_nvp("default_neuron_size", m_default_neuron_size)
	   & make_nvp("restrict_rightmost_neuron_blocks", m_restrict_rightmost_neuron_blocks)
	   & make_nvp("minimize_number_of_sending_repeaters", m_minimize_number_of_sending_repeaters)
	   & make_nvp("skip_hicanns_without_neuron_blacklisting", m_skip_hicanns_without_neuron_blacklisting);
	// clang-format on
}

} // namespace parameters
} // namespace placement
} // namespace marocco

BOOST_CLASS_EXPORT_IMPLEMENT(::marocco::placement::parameters::NeuronPlacement)

#include "boost/serialization/serialization_helper.tcc"
EXPLICIT_INSTANTIATE_BOOST_SERIALIZE(::marocco::placement::parameters::NeuronPlacement)
