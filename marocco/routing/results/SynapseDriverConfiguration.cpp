#include "marocco/routing/results/SynapseDriverConfiguration.h"

#include <boost/serialization/map.hpp>
#include <boost/serialization/nvp.hpp>

namespace marocco {
namespace routing {
namespace results {

SynapseDriverConfiguration::SynapseDriverConfiguration()
	: m_stp_mode(STPMode::off)
{
}

STPMode const& SynapseDriverConfiguration::stp_mode() const
{
	return m_stp_mode;
}

void SynapseDriverConfiguration::set_stp_mode(STPMode const& mode)
{
	m_stp_mode = mode;
}

template <typename Archiver>
void SynapseDriverConfiguration::serialize(Archiver& ar, const unsigned int /* version */)
{
	using namespace boost::serialization;
	// clang-format off
	ar & make_nvp("stp_mode", m_stp_mode);
	// clang-format on
}

} // namespace results
} // namespace routing
} // namespace marocco

BOOST_CLASS_EXPORT_IMPLEMENT(::marocco::routing::results::SynapseDriverConfiguration)

#include "boost/serialization/serialization_helper.tcc"
EXPLICIT_INSTANTIATE_BOOST_SERIALIZE(::marocco::routing::results::SynapseDriverConfiguration)
