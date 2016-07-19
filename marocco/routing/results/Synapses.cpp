#include "marocco/routing/results/Synapses.h"

#include <boost/serialization/nvp.hpp>
#include <boost/serialization/optional.hpp>

using namespace HMF::Coordinate;
using boost::multi_index::get;

namespace marocco {
namespace routing {
namespace results {

Synapses::item_type::item_type()
    : m_projection(0u), m_source_neuron(), m_target_neuron(), m_hardware_synapse()
{
}

Synapses::item_type::item_type(
	projection_type projection,
	BioNeuron const& source_neuron,
	BioNeuron const& target_neuron,
	hardware_synapse_type const& hardware_synapse)
	: m_projection(projection),
	  m_source_neuron(source_neuron),
	  m_target_neuron(target_neuron),
	  m_hardware_synapse(hardware_synapse)
{
}

Synapses::item_type::item_type(
	projection_type projection,
	BioNeuron const& source_neuron,
	BioNeuron const& target_neuron)
	: m_projection(projection),
	  m_source_neuron(source_neuron),
	  m_target_neuron(target_neuron)
{
}

auto Synapses::item_type::projection() const -> projection_type
{
	return m_projection;
}

BioNeuron const& Synapses::item_type::source_neuron() const
{
	return m_source_neuron;
}

BioNeuron const& Synapses::item_type::target_neuron() const
{
	return m_target_neuron;
}

auto Synapses::item_type::hardware_synapse() const -> optional_hardware_synapse_type const&
{
	return m_hardware_synapse;
}

void Synapses::add(
	projection_type projection,
	BioNeuron const& source_neuron,
	BioNeuron const& target_neuron,
	hardware_synapse_type const& hardware_synapse)
{
	// Check if this hardware synapse is already in use.  As `boost::none` is used to
	// represent synapse loss, we have to use `hashed_non_unique` instead of `hashed_unique`,
	// necessitating this additional check.
	auto const& by_hardware_synapse = get<hardware_synapse_type>(m_container);
	if (by_hardware_synapse.find(hardware_synapse) != by_hardware_synapse.end()) {
		throw std::runtime_error("hardware synapse already in use");
	}

	if (!m_container.insert(item_type(projection, source_neuron, target_neuron, hardware_synapse))
		.second) {
		throw std::runtime_error("error when adding synapse routing result");
	}
}

void Synapses::add_unrealized_synapse(
	projection_type projection,
	BioNeuron const& source_neuron,
	BioNeuron const& target_neuron)
{
	// Check if this connection already has associated hardware synapses.
	auto const& by_projection_and_neurons = get<projection_type>(m_container);
	if (by_projection_and_neurons.find(
			boost::make_tuple(projection, source_neuron, target_neuron)) !=
		by_projection_and_neurons.end()) {
		throw std::runtime_error("conflict when adding synapse loss");
	}

	if (!m_container.insert(item_type(projection, source_neuron, target_neuron)).second) {
		throw std::runtime_error("error when adding synapse loss");
	}
}

auto Synapses::find(BioNeuron const& source_neuron, BioNeuron const& target_neuron) const
    -> iterable<by_neurons_type::iterator>
{
	return make_iterable(
		get<BioNeuron>(m_container).equal_range(boost::make_tuple(source_neuron, target_neuron)));
}

auto Synapses::find(
	projection_type projection,
	BioNeuron const& source_neuron,
	BioNeuron const& target_neuron) const -> iterable<by_projection_and_neurons_type::iterator>
{
	return make_iterable(
		get<projection_type>(m_container)
			.equal_range(boost::make_tuple(projection, source_neuron, target_neuron)));
}

auto Synapses::find(SynapseOnWafer const& hardware_synapse) const
    -> iterable<by_hardware_synapse_type::iterator>
{
	return make_iterable(get<hardware_synapse_type>(m_container).equal_range(hardware_synapse));
}

auto Synapses::find(HICANNOnWafer const& hicann, SynapseRowOnHICANN const& synapse_row) const
	-> iterable<by_hardware_synapse_type::iterator>
{
	return make_iterable(
		get<hardware_synapse_type>(m_container)
			.lower_bound(
				SynapseOnWafer(
					SynapseOnHICANN(synapse_row, SynapseColumnOnHICANN(SynapseColumnOnHICANN::min)),
					hicann)),
		get<hardware_synapse_type>(m_container)
			.upper_bound(
				SynapseOnWafer(
					SynapseOnHICANN(synapse_row, SynapseColumnOnHICANN(SynapseColumnOnHICANN::max)),
					hicann)));
}

auto Synapses::unrealized_synapses() const -> iterable<by_hardware_synapse_type::iterator>
{
	return make_iterable(get<hardware_synapse_type>(m_container).equal_range(boost::none));
}

bool Synapses::empty() const
{
	return m_container.empty();
}

size_t Synapses::size() const
{
	return m_container.size();
}

auto Synapses::begin() const -> iterator {
	return m_container.begin();
}

auto Synapses::end() const -> iterator {
	return m_container.end();
}

template <typename Archiver>
void Synapses::item_type::serialize(Archiver& ar, const unsigned int /* version */)
{
	using namespace boost::serialization;
	// clang-format off
	ar & make_nvp("projection", m_projection)
	   & make_nvp("source_neuron", m_source_neuron)
	   & make_nvp("target_neuron", m_target_neuron)
	   & make_nvp("hardware_synapse", m_hardware_synapse);
	// clang-format on
}

template <typename Archiver>
void Synapses::serialize(Archiver& ar, const unsigned int /* version */)
{
	using namespace boost::serialization;
	// clang-format off
	ar & make_nvp("container", m_container);
	// clang-format on
}

} // namespace results
} // namespace routing
} // namespace marocco

BOOST_CLASS_EXPORT_IMPLEMENT(::marocco::routing::results::Synapses)
BOOST_CLASS_EXPORT_IMPLEMENT(::marocco::routing::results::Synapses::item_type)

#include "boost/serialization/serialization_helper.tcc"
EXPLICIT_INSTANTIATE_BOOST_SERIALIZE(::marocco::routing::results::Synapses)
EXPLICIT_INSTANTIATE_BOOST_SERIALIZE(::marocco::routing::results::Synapses::item_type)
