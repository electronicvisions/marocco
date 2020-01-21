#include "marocco/routing/results/ConnectedSynapseDrivers.h"

#include <iostream>
#include <stdexcept>
#include <boost/serialization/nvp.hpp>
#include <boost/serialization/set.hpp>

#include "halco/hicann/v2/quadrant.h"

using namespace halco::hicann::v2;

namespace marocco {
namespace routing {
namespace results {

ConnectedSynapseDrivers::ConnectedSynapseDrivers()
	: m_primary_driver(), m_drivers{m_primary_driver}
{
}

ConnectedSynapseDrivers::ConnectedSynapseDrivers(SynapseDriverOnHICANN const& primary_driver)
	: m_primary_driver(primary_driver), m_drivers{primary_driver}
{
}

ConnectedSynapseDrivers::ConnectedSynapseDrivers(
	coordinate_type const& primary_driver,
	coordinate_type const& first_driver,
	coordinate_type const& last_driver)
	: ConnectedSynapseDrivers(primary_driver)
{
	if (first_driver > primary_driver) {
		throw std::invalid_argument(
			"first driver has to be above primary driver");
	}

	if (last_driver < primary_driver) {
		throw std::invalid_argument(
			"last driver has to be below primary driver");
	}

	connect(first_driver);
	connect(last_driver);
}

void ConnectedSynapseDrivers::connect(SynapseDriverOnQuadrant const& driver)
{
	size_t lower = driver;
	size_t upper = m_primary_driver.toSynapseDriverOnQuadrant();
	if (lower > upper) {
		std::swap(lower, upper);
	}

	auto const quadrant = m_primary_driver.toQuadrantOnHICANN();
	for (size_t ii = lower; ii <= upper; ++ii) {
		m_drivers.insert(SynapseDriverOnQuadrant(ii).toSynapseDriverOnHICANN(quadrant));
	}
}

void ConnectedSynapseDrivers::connect(SynapseDriverOnHICANN const& driver)
{
	auto const quadrant = m_primary_driver.toQuadrantOnHICANN();

	if (driver.toQuadrantOnHICANN() != quadrant) {
		throw std::invalid_argument(
			"quadrant of driver does not match primary driver");
	}

	connect(driver.toSynapseDriverOnQuadrant());
}

SynapseDriverOnHICANN const& ConnectedSynapseDrivers::primary_driver() const
{
	return m_primary_driver;
}

auto ConnectedSynapseDrivers::drivers() const -> container_type const&
{
	return m_drivers;
}

size_t ConnectedSynapseDrivers::size() const
{
	return m_drivers.size();
}

std::ostream& operator<<(std::ostream& os, ConnectedSynapseDrivers const& connected_drivers)
{
	os << "ConnectedSynapseDrivers("
	   << connected_drivers.primary_driver();
	if (connected_drivers.size() > 1) {
		auto const& drivers = connected_drivers.drivers();
		os << ", " << *drivers.begin() << ", " << *drivers.rbegin();
	}
	os << ")";
	return os;
}

template <typename Archiver>
void ConnectedSynapseDrivers::serialize(Archiver& ar, const unsigned int /* version */)
{
	using namespace boost::serialization;
	// clang-format off
	ar & make_nvp("primary_driver", m_primary_driver)
	   & make_nvp("drivers", m_drivers);
	// clang-format on
}

} // namespace results
} // namespace routing
} // namespace marocco

BOOST_CLASS_EXPORT_IMPLEMENT(::marocco::routing::results::ConnectedSynapseDrivers)

#include "boost/serialization/serialization_helper.tcc"
EXPLICIT_INSTANTIATE_BOOST_SERIALIZE(::marocco::routing::results::ConnectedSynapseDrivers)
