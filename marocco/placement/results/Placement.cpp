#include "marocco/placement/results/Placement.h"

#include <boost/serialization/nvp.hpp>
#include <boost/serialization/optional.hpp>

// definition of `hash_value(boost::optional<T> const&)` from lib-boost-patches (c/1573)
#include "boost/optional/hash_value.tcc"

using namespace HMF::Coordinate;
using boost::multi_index::get;

namespace marocco {
namespace placement {
namespace results {

Placement::item_type::item_type()
	: m_bio_neuron(BioNeuron(0, 0)),
	  m_logical_neuron(LogicalNeuron::external(0)),
	  m_neuron_block(boost::none),
	  m_address(boost::none)
{
}

Placement::item_type::item_type(
    BioNeuron const& bio_neuron, LogicalNeuron const& logical_neuron)
	: m_bio_neuron(bio_neuron),
	  m_logical_neuron(logical_neuron),
	  m_neuron_block(
		  logical_neuron.is_external()
			  ? boost::none
			  : neuron_block_type{logical_neuron.front().toNeuronBlockOnWafer()}),
	  m_address(boost::none)
{
}

auto Placement::item_type::neuron_block() const -> neuron_block_type const&
{
	return m_neuron_block;
}

auto Placement::item_type::dnc_merger() const -> dnc_merger_type
{
	if (m_address == boost::none) {
		return boost::none;
	}
	return {m_address->toDNCMergerOnWafer()};
}

auto Placement::item_type::population() const -> vertex_descriptor
{
	return m_bio_neuron.population();
}

auto Placement::item_type::neuron_index() const -> vertex_descriptor
{
	return m_bio_neuron.neuron_index();
}

BioNeuron const& Placement::item_type::bio_neuron() const
{
	return m_bio_neuron;
}

LogicalNeuron const& Placement::item_type::logical_neuron() const
{
	return m_logical_neuron;
}

auto Placement::item_type::address() const -> address_type const&
{
	return m_address;
}

void Placement::item_type::set_address(L1AddressOnWafer const& address)
{
	m_address = address;
}

void Placement::add(BioNeuron const& bio_neuron, LogicalNeuron const& logical_neuron)
{
	if (!m_container.insert(item_type(bio_neuron, logical_neuron)).second) {
		throw std::runtime_error("conflict when adding neuron placement result");
	}
}

auto Placement::find(BioNeuron const& bio_neuron) const
	-> iterable<by_bio_neuron_type::iterator>
{
	return make_iterable(get<BioNeuron>(m_container).equal_range(bio_neuron));
}

auto Placement::find(LogicalNeuron const& logical_neuron) const
	-> iterable<by_logical_neuron_type::iterator>
{
	return make_iterable(get<LogicalNeuron>(m_container).equal_range(logical_neuron));
}

auto Placement::find(vertex_descriptor const& population) const
	-> iterable<by_population_type::iterator>
{
	return make_iterable(get<vertex_descriptor>(m_container).equal_range(population));
}

auto Placement::find(
	boost::optional<NeuronBlockOnWafer> const& neuron_block) const
	-> iterable<by_neuron_block_type::iterator>
{
	return make_iterable(get<NeuronBlockOnWafer>(m_container).equal_range(neuron_block));
}

auto Placement::find(
	boost::optional<HMF::Coordinate::DNCMergerOnWafer> const& dnc_merger) const
	-> iterable<by_dnc_merger_type::iterator>
{
	return make_iterable(get<DNCMergerOnWafer>(m_container).equal_range(dnc_merger));
}

auto Placement::find(NeuronBlockOnWafer const& neuron_block) const
	-> iterable<by_neuron_block_type::iterator>
{
	return make_iterable(get<NeuronBlockOnWafer>(m_container).equal_range(neuron_block));
}

auto Placement::find(
	HMF::Coordinate::DNCMergerOnWafer const& dnc_merger) const
	-> iterable<by_dnc_merger_type::iterator>
{
	return make_iterable(get<DNCMergerOnWafer>(m_container).equal_range(dnc_merger));
}

auto Placement::find(HICANNOnWafer const& hicann) const
	-> iterable<by_neuron_block_type::iterator>
{
	return make_iterable(
		get<NeuronBlockOnWafer>(m_container)
			.lower_bound(NeuronBlockOnWafer(NeuronBlockOnHICANN(NeuronBlockOnHICANN::min), hicann)),
		get<NeuronBlockOnWafer>(m_container)
			.upper_bound(
				NeuronBlockOnWafer(NeuronBlockOnHICANN(NeuronBlockOnHICANN::max), hicann)));
}

bool Placement::empty() const
{
	return m_container.empty();
}

size_t Placement::size() const
{
	return m_container.size();
}

auto Placement::begin() const -> iterator {
	return m_container.begin();
}

auto Placement::end() const -> iterator {
	return m_container.end();
}

void Placement::set_address(
    LogicalNeuron const& logical_neuron, L1AddressOnWafer const& address)
{
	auto& by_neuron = get<LogicalNeuron>(m_container);
	auto it = by_neuron.find(logical_neuron);
	if (it == by_neuron.end()) {
		throw std::out_of_range("specified logical neuron not found in placement result");
	}
	if (!by_neuron.modify(it, [&address](item_type& item) { item.set_address(address); })) {
		throw std::runtime_error("could not store L1 address of neuron");
	}
}

template <typename Archiver>
void Placement::item_type::serialize(Archiver& ar, const unsigned int /* version */)
{
	using namespace boost::serialization;
	// clang-format off
	ar & make_nvp("bio_neuron", m_bio_neuron)
	   & make_nvp("logical_neuron", m_logical_neuron)
	   & make_nvp("neuron_block", m_neuron_block)
	   & make_nvp("address", m_address);
	// clang-format on
}

template <typename Archiver>
void Placement::serialize(Archiver& ar, const unsigned int /* version */)
{
	using namespace boost::serialization;
	// clang-format off
	ar & make_nvp("container", m_container);
	// clang-format on
}

} // namespace results
} // namespace placement
} // namespace marocco

BOOST_CLASS_EXPORT_IMPLEMENT(::marocco::placement::results::Placement)
BOOST_CLASS_EXPORT_IMPLEMENT(::marocco::placement::results::Placement::item_type)

#include "boost/serialization/serialization_helper.tcc"
EXPLICIT_INSTANTIATE_BOOST_SERIALIZE(::marocco::placement::results::Placement)
EXPLICIT_INSTANTIATE_BOOST_SERIALIZE(::marocco::placement::results::Placement::item_type)
