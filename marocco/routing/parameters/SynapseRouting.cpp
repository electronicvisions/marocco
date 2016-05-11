#include "marocco/routing/parameters/SynapseRouting.h"

#include <stdexcept>
#include <boost/serialization/nvp.hpp>

#include "hal/Coordinate/Synapse.h"

using namespace HMF::Coordinate;

namespace marocco {
namespace routing {
namespace parameters {

SynapseRouting::SynapseRouting()
	: m_driver_chain_length(SynapseDriverOnQuadrant::size), m_only_allow_background_events(false)
{
}

void SynapseRouting::driver_chain_length(size_t value)
{
	if (m_driver_chain_length > SynapseDriverOnQuadrant::size) {
		throw std::invalid_argument(
			"maximum chain length exceeds limits imposed by hardware geometry");
	}
	m_driver_chain_length = value;
}

size_t SynapseRouting::driver_chain_length() const
{
	return m_driver_chain_length;
}

void SynapseRouting::only_allow_background_events(bool enable)
{
	m_only_allow_background_events = enable;
}

bool SynapseRouting::only_allow_background_events() const
{
	return m_only_allow_background_events;
}

template <typename Archive>
void SynapseRouting::serialize(Archive& ar, unsigned int const /* version */)
{
	using namespace boost::serialization;
	// clang-format off
	ar & make_nvp("driver_chain_length", m_driver_chain_length)
	   & make_nvp("only_allow_background_events", m_only_allow_background_events);
	// clang-format on
}

} // namespace parameters
} // namespace routing
} // namespace marocco

BOOST_CLASS_EXPORT_IMPLEMENT(::marocco::routing::parameters::SynapseRouting)

#include "boost/serialization/serialization_helper.tcc"
EXPLICIT_INSTANTIATE_BOOST_SERIALIZE(::marocco::routing::parameters::SynapseRouting)
