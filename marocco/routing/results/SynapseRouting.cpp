#include "marocco/routing/results/SynapseRouting.h"

#include <boost/serialization/map.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/serialization/unordered_map.h>

using namespace halco::hicann::v2;

namespace marocco {
namespace routing {
namespace results {

SynapseRouting::synapse_switches_item_type::synapse_switches_item_type(
	source_type const& source, ConnectedSynapseDrivers const& connected_drivers)
	: m_source(source), m_connected_drivers(connected_drivers)
{
}

SynapseRouting::synapse_switches_item_type::synapse_switches_item_type()
	: m_source(), m_connected_drivers()
{
}

auto SynapseRouting::synapse_switches_item_type::source() const -> source_type const&
{
	return m_source;
}

ConnectedSynapseDrivers const& SynapseRouting::synapse_switches_item_type::connected_drivers() const
{
	return m_connected_drivers;
}

bool SynapseRouting::HICANN::has(SynapseDriverOnHICANN const& driver) const
{
	return m_synapse_driver_configurations.find(driver) != m_synapse_driver_configurations.end();
}

bool SynapseRouting::HICANN::has(SynapseRowOnHICANN const& row) const
{
	return m_synapse_rows.find(row) != m_synapse_rows.end();
}

SynapseDriverConfiguration& SynapseRouting::HICANN::operator[](
    halco::hicann::v2::SynapseDriverOnHICANN const& driver)
{
	return m_synapse_driver_configurations[driver];
}

SynapseDriverConfiguration const& SynapseRouting::HICANN::operator[](
    halco::hicann::v2::SynapseDriverOnHICANN const& driver) const
{
	return m_synapse_driver_configurations.at(driver);
}

SynapseRowConfiguration& SynapseRouting::HICANN::operator[](
    halco::hicann::v2::SynapseRowOnHICANN const& row)
{
	return m_synapse_rows[row];
}

SynapseRowConfiguration const& SynapseRouting::HICANN::operator[](
    halco::hicann::v2::SynapseRowOnHICANN const& row) const
{
	return m_synapse_rows.at(row);
}

void SynapseRouting::HICANN::add_synapse_switch(
	halco::hicann::v2::VLineOnHICANN const& vline,
	ConnectedSynapseDrivers const& drivers)
{
	m_synapse_switches.insert(synapse_switches_item_type(vline, drivers));
}

auto SynapseRouting::HICANN::operator[](halco::hicann::v2::VLineOnHICANN const& vline) const
	-> iterable<synapse_switches_type::iterator>
{
	return make_iterable(m_synapse_switches.equal_range(vline));
}

auto SynapseRouting::HICANN::synapse_switches() const
	-> iterable<synapse_switches_type::iterator>
{
	return make_iterable(m_synapse_switches.begin(), m_synapse_switches.end());
}

SynapticInputs& SynapseRouting::HICANN::synaptic_inputs()
{
	return m_synaptic_inputs;
}

SynapticInputs const& SynapseRouting::HICANN::synaptic_inputs() const
{
	return m_synaptic_inputs;
}

bool SynapseRouting::has(halco::hicann::v2::HICANNOnWafer const& hicann) const{
	return m_hicanns.find(hicann) != m_hicanns.end();
}

auto SynapseRouting::operator[](halco::hicann::v2::HICANNOnWafer const& hicann) -> HICANN&
{
	return m_hicanns[hicann];
}

auto SynapseRouting::operator[](halco::hicann::v2::HICANNOnWafer const& hicann) const -> HICANN const&
{
	return m_hicanns.at(hicann);
}

Synapses& SynapseRouting::synapses()
{
	return m_synapses;
}

Synapses const& SynapseRouting::synapses() const
{
	return m_synapses;
}

template <typename Archiver>
void SynapseRouting::synapse_switches_item_type::serialize(Archiver& ar, const unsigned int /* version */)
{
	using namespace boost::serialization;
	// clang-format off
	ar & make_nvp("source", m_source)
	   & make_nvp("connected_drivers", m_connected_drivers);
	// clang-format on
}

template <typename Archiver>
void SynapseRouting::HICANN::serialize(Archiver& ar, const unsigned int /* version */)
{
	using namespace boost::serialization;
	// clang-format off
	ar & make_nvp("synapse_driver_configurations", m_synapse_driver_configurations)
	   & make_nvp("synapse_rows", m_synapse_rows)
	   & make_nvp("synapse_switches", m_synapse_switches)
	   & make_nvp("synaptic_inputs", m_synaptic_inputs);
	// clang-format on
}

template <typename Archiver>
void SynapseRouting::serialize(Archiver& ar, const unsigned int /* version */)
{
	using namespace boost::serialization;
	// clang-format off
	ar & make_nvp("hicanns", m_hicanns)
	   & make_nvp("synapses", m_synapses);
	// clang-format on
}

} // namespace results
} // namespace routing
} // namespace marocco

BOOST_CLASS_EXPORT_IMPLEMENT(::marocco::routing::results::SynapseRouting)
BOOST_CLASS_EXPORT_IMPLEMENT(
	::marocco::routing::results::SynapseRouting::synapse_switches_item_type)
BOOST_CLASS_EXPORT_IMPLEMENT(::marocco::routing::results::SynapseRouting::HICANN)

#include "boost/serialization/serialization_helper.tcc"
EXPLICIT_INSTANTIATE_BOOST_SERIALIZE(::marocco::routing::results::SynapseRouting)
EXPLICIT_INSTANTIATE_BOOST_SERIALIZE(
	::marocco::routing::results::SynapseRouting::synapse_switches_item_type)
EXPLICIT_INSTANTIATE_BOOST_SERIALIZE(::marocco::routing::results::SynapseRouting::HICANN)
