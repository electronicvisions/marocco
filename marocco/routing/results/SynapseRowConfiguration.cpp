#include "marocco/routing/results/SynapseRowConfiguration.h"

#include <boost/serialization/nvp.hpp>

using namespace halco::hicann::v2;
using namespace halco::common;

namespace marocco {
namespace routing {
namespace results {

SynapseRowConfiguration::SynapseRowConfiguration()
	: m_synaptic_input(), m_address()
{
}

SynapseRowConfiguration::SynapseRowConfiguration(SynapticInputOnNeuron const& synaptic_input)
	: m_synaptic_input(synaptic_input), m_address()
{
}

auto SynapseRowConfiguration::address(Parity const& parity) const -> address_type const&
{
	return m_address[parity];
}

void SynapseRowConfiguration::set_address(Parity const& parity, address_type const& address)
{
	m_address[parity] = address;
}

SynapticInputOnNeuron const& SynapseRowConfiguration::synaptic_input() const
{
	return m_synaptic_input;
}

void SynapseRowConfiguration::set_synaptic_input(SynapticInputOnNeuron const& synaptic_input)
{
	m_synaptic_input = synaptic_input;
}

template <typename Archiver>
void SynapseRowConfiguration::serialize(Archiver& ar, const unsigned int /* version */)
{
	using namespace boost::serialization;
	// clang-format off
	ar & make_nvp("synaptic_input", m_synaptic_input)
	   & make_nvp("address", m_address);
	// clang-format on
}

} // namespace results
} // namespace routing
} // namespace marocco

BOOST_CLASS_EXPORT_IMPLEMENT(::marocco::routing::results::SynapseRowConfiguration)

#include "boost/serialization/serialization_helper.tcc"
EXPLICIT_INSTANTIATE_BOOST_SERIALIZE(::marocco::routing::results::SynapseRowConfiguration)
