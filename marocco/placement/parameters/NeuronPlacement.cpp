#include "marocco/placement/parameters/NeuronPlacement.h"

#include <boost/serialization/nvp.hpp>

#include "halco/hicann/v2/neuron.h"
#include "marocco/coordinates/LogicalNeuron.h"

#include "marocco/placement/algorithms/byNeuronBlockEnumAndPopulationIDasc.h"

namespace marocco {
namespace placement {
namespace parameters {

NeuronPlacement::NeuronPlacement() :
    m_default_neuron_size(4),
    m_restrict_rightmost_neuron_blocks(false),
    m_minimize_number_of_sending_repeaters(false),
    m_default_placement_strategy(
        boost::make_shared<algorithms::byNeuronBlockEnumAndPopulationIDasc>())
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

void NeuronPlacement::skip_hicanns_without_neuron_blacklisting(bool /*enable*/)
{
	throw std::runtime_error(
	    "NeuronPlacement::skip_hicanns_without_neuron_blacklisting(bool) is deprecated");
}

bool NeuronPlacement::skip_hicanns_without_neuron_blacklisting() const
{
	throw std::runtime_error(
	    "NeuronPlacement::skip_hicanns_without_neuron_blacklisting() is deprecated");
}

void NeuronPlacement::default_placement_strategy(
    boost::shared_ptr<algorithms::PlacePopulationsBase> const placer)
{
	m_default_placement_strategy = placer;
}

boost::shared_ptr<algorithms::PlacePopulationsBase> NeuronPlacement::default_placement_strategy()
    const
{
	return m_default_placement_strategy;
}

template <typename Archive>
void NeuronPlacement::serialize(Archive& ar, unsigned int const version)
{
	using namespace boost::serialization;
	// clang-format off
	ar & make_nvp("default_neuron_size", m_default_neuron_size)
	   & make_nvp("restrict_rightmost_neuron_blocks", m_restrict_rightmost_neuron_blocks)
	   & make_nvp("minimize_number_of_sending_repeaters", m_minimize_number_of_sending_repeaters);

	if (version == 0) {
		bool dummy = false;
		ar & make_nvp("skip_hicanns_without_neuron_blacklisting", dummy);
	};

	ar & make_nvp("default_placement_strategy", *m_default_placement_strategy);
	// clang-format on
}

} // namespace parameters
} // namespace placement
} // namespace marocco

BOOST_CLASS_EXPORT_IMPLEMENT(::marocco::placement::parameters::NeuronPlacement)

#include "boost/serialization/serialization_helper.tcc"
EXPLICIT_INSTANTIATE_BOOST_SERIALIZE(::marocco::placement::parameters::NeuronPlacement)
