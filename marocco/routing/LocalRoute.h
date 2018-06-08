#pragma once

#include <vector>
#include <set>
#include <map>
#include <functional>

#include "marocco/routing/Route.h"
#include "hal/Coordinate/HMFGeometry.h"

namespace marocco {
namespace routing {

/// LocalRoute represents a route which ends at a given hicann on a given bus
/// segment with a given number of sources, which needs synapse driver resource
/// to be allocated for it.
class LocalRoute
{
public:
	LocalRoute(Route const& route, Route::BusSegment const& bus);

	// cached number of sources sending l1 events via this route
	size_t numSources() const;

	L1Bus const& targetBus(routing_graph const& rgraph) const;
	Route::BusSegment targetBus() const;

	Route const& route() const;

	int id() const;

	// for the synapse row mapping
	typedef assignment::AddressMapping::address_type l1_address_t;
	typedef HMF::Coordinate::SynapseRowOnHICANN      synapse_row_t;
	typedef HMF::Coordinate::SynapseDriverOnHICANN   synapse_driver_t;

private:
	// cache of mRoute->numSources() to improve performance of operator<
	size_t mNumSources;

	// local incoming vertical bus
	Route::BusSegment mBus;

	// reference to the corresponding route
	Route mRoute;

	// for serialization purposes
	LocalRoute();

	friend class SynapseRowSource;
	friend class boost::serialization::access;
	template<typename Archiver>
	void serialize(Archiver& ar, unsigned int const /*version*/);
};

} // namespace routing
} // namespace marocco

namespace marocco {
namespace routing {

template<typename Archiver>
void LocalRoute::serialize(Archiver& ar, unsigned int const /*version*/)
{
	using namespace boost::serialization;
	ar & make_nvp(          "bus", mBus)
	   & make_nvp(        "route", mRoute)
	   & make_nvp(   "numSources", mNumSources);
}

} // namespace routing
} // namespace marocco

namespace std {

template<>
struct hash<marocco::routing::LocalRoute>
{
	size_t operator() (marocco::routing::LocalRoute const& lr) const
	{
		return lr.id();
	}
};

} // namespace std
