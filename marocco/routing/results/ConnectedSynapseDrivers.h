#pragma once

#include <iosfwd>
#include <set>
#include <boost/serialization/export.hpp>

#include "halco/hicann/v2/synapse.h"

namespace boost {
namespace serialization {
class access;
} // namespace serialization
} // namespace boost

namespace marocco {
namespace routing {
namespace results {

class ConnectedSynapseDrivers {
public:
	typedef halco::hicann::v2::SynapseDriverOnHICANN coordinate_type;
	typedef std::set<coordinate_type> container_type;

	ConnectedSynapseDrivers();
	ConnectedSynapseDrivers(coordinate_type const& primary_driver);
	ConnectedSynapseDrivers(
		coordinate_type const& primary_driver,
		coordinate_type const& first_driver,
		coordinate_type const& last_driver);

	/**
	 * @brief Add specified synapse driver and all intermediate drivers.
	 */
	void connect(halco::hicann::v2::SynapseDriverOnQuadrant const& driver);

	/**
	 * @brief Add specified synapse driver and all intermediate drivers.
	 * @throw std::invalid_argument If \c driver does not lie in the same quadrant of the HICANN.
	 */
	void connect(coordinate_type const& driver);

	coordinate_type const& primary_driver() const;

	container_type const& drivers() const;

	size_t size() const;

private:
	coordinate_type m_primary_driver;
	container_type m_drivers;

	friend class boost::serialization::access;
	template <typename Archiver>
	void serialize(Archiver& ar, const unsigned int /* version */);
}; // ConnectedSynapseDrivers

std::ostream& operator<<(std::ostream& os, ConnectedSynapseDrivers const& connected_drivers);

} // namespace results
} // namespace routing
} // namespace marocco

BOOST_CLASS_EXPORT_KEY(::marocco::routing::results::ConnectedSynapseDrivers)
