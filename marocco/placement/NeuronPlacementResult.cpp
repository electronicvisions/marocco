#include "marocco/placement/NeuronPlacementResult.h"

using namespace HMF::Coordinate;
using boost::multi_index::get;

namespace marocco {
namespace placement {

NeuronPlacementResult::item_type::item_type(
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

auto NeuronPlacementResult::item_type::neuron_block() const -> neuron_block_type const&
{
	return m_neuron_block;
}

auto NeuronPlacementResult::item_type::dnc_merger() const -> dnc_merger_type
{
	if (m_address == boost::none) {
		return boost::none;
	}
	return {m_address->toDNCMergerOnWafer()};
}

auto NeuronPlacementResult::item_type::population() const -> vertex_descriptor
{
	return m_bio_neuron.population();
}

auto NeuronPlacementResult::item_type::neuron_index() const -> vertex_descriptor
{
	return m_bio_neuron.neuron_index();
}

BioNeuron const& NeuronPlacementResult::item_type::bio_neuron() const
{
	return m_bio_neuron;
}

LogicalNeuron const& NeuronPlacementResult::item_type::logical_neuron() const
{
	return m_logical_neuron;
}

auto NeuronPlacementResult::item_type::address() const -> address_type const&
{
	return m_address;
}

void NeuronPlacementResult::item_type::set_address(address_type const& address)
{
	m_address = address;
}

auto NeuronPlacementResult::denmem_assignment() const -> denmem_assignment_type const&
{
	return m_denmem_assignment;
}
auto NeuronPlacementResult::denmem_assignment() -> denmem_assignment_type&
{
	return m_denmem_assignment;
}

auto NeuronPlacementResult::primary_denmems_for_population() const
	-> primary_denmems_for_population_type const&
{
	return m_primaries;
}

auto NeuronPlacementResult::primary_denmems_for_population() -> primary_denmems_for_population_type&
{
	return m_primaries;
}

void NeuronPlacementResult::add(BioNeuron const& bio_neuron, LogicalNeuron const& logical_neuron)
{
	if (!m_container.insert(item_type(bio_neuron, logical_neuron)).second) {
		throw std::runtime_error("conflict when adding neuron placement result");
	}
}

auto NeuronPlacementResult::find(BioNeuron const& bio_neuron) const
	-> iterable<by_bio_neuron_type::iterator>
{
	return make_iterable(get<BioNeuron>(m_container).equal_range(bio_neuron));
}

auto NeuronPlacementResult::find(LogicalNeuron const& logical_neuron) const
	-> iterable<by_logical_neuron_type::iterator>
{
	return make_iterable(get<LogicalNeuron>(m_container).equal_range(logical_neuron));
}

auto NeuronPlacementResult::find(vertex_descriptor const& population) const
	-> iterable<by_population_type::iterator>
{
	return make_iterable(get<vertex_descriptor>(m_container).equal_range(population));
}

auto NeuronPlacementResult::find(
	boost::optional<NeuronBlockOnWafer> const& neuron_block) const
	-> iterable<by_neuron_block_type::iterator>
{
	return make_iterable(get<NeuronBlockOnWafer>(m_container).equal_range(neuron_block));
}

auto NeuronPlacementResult::find(
	boost::optional<HMF::Coordinate::DNCMergerOnWafer> const& dnc_merger) const
	-> iterable<by_dnc_merger_type::iterator>
{
	return make_iterable(get<DNCMergerOnWafer>(m_container).equal_range(dnc_merger));
}

auto NeuronPlacementResult::find(NeuronBlockOnWafer const& neuron_block) const
	-> iterable<by_neuron_block_type::iterator>
{
	return make_iterable(get<NeuronBlockOnWafer>(m_container).equal_range(neuron_block));
}

auto NeuronPlacementResult::find(
	HMF::Coordinate::DNCMergerOnWafer const& dnc_merger) const
	-> iterable<by_dnc_merger_type::iterator>
{
	return make_iterable(get<DNCMergerOnWafer>(m_container).equal_range(dnc_merger));
}

auto NeuronPlacementResult::begin() const -> by_population_type::iterator {
	return get<vertex_descriptor>(m_container).begin();
}

auto NeuronPlacementResult::end() const -> by_population_type::iterator {
	return get<vertex_descriptor>(m_container).end();
}

void NeuronPlacementResult::set_address(
    LogicalNeuron const& logical_neuron, item_type::address_type const& address)
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

} // namespace placement
} // namespace marocco
