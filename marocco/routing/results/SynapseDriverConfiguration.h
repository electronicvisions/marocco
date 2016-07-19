#pragma once

#include <boost/serialization/export.hpp>

#include "marocco/routing/STPMode.h"

namespace boost {
namespace serialization {
class access;
} // namespace serialization
} // namespace boost

namespace marocco {
namespace routing {
namespace results {

class SynapseDriverConfiguration
{
public:
	SynapseDriverConfiguration();

	STPMode const& stp_mode() const;

	void set_stp_mode(STPMode const& mode);

private:
	STPMode m_stp_mode;

	friend class boost::serialization::access;
	template <typename Archiver>
	void serialize(Archiver& ar, const unsigned int /* version */);
}; // SynapseDriverConfiguration

} // namespace results
} // namespace routing
} // namespace marocco

BOOST_CLASS_EXPORT_KEY(::marocco::routing::results::SynapseDriverConfiguration)
