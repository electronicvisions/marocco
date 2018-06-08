#pragma once

#include <set>
#include <marocco/config.h>

namespace marocco {
namespace routing {

/** container to store the synapse drivers assigned to one VLine.
 * also holds the primary driver, which is connected to the VLine
 *
 * TODO: is the primary driver also in the list of all drivers?
 */
struct DriverAssignment
{
	typedef HMF::Coordinate::SynapseDriverOnHICANN Driver;

	/// This list of SynapseDrivers is a set, just to make sure, they are sorted
	/// from top to bottom.
	typedef std::set<Driver> Drivers;

	Driver  primary;
	Drivers drivers;
};

} // namespace routing
} // namespace marocco
