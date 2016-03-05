#pragma once

#include <vector>
#include <map>
#include <boost/serialization/nvp.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/shared_ptr.hpp>

#include "marocco/config.h"

#include "marocco/routing/HardwareProjection.h"
#include "marocco/routing/routing_graph.h"

namespace marocco {
namespace routing {

// fwd decl
class LocalRoute;

/// @class Route class
///
/// @brief hardware route representation
///
/// In principle a route is defined by its corresponding neural projection, the
/// the hardware source and hardware targets are then given by the placement and
/// the routes realization is ultimately described by a set of edges in the
/// switchbox graph corresponding to routing resources
class Route
{
public:
	/// edge type in switchbox graph
	typedef routing_graph::vertex_descriptor BusSegment;

	/// route_type, key type is target HICANN and vector contains predecessor list.
	// FIXME: use as soon as python converter works
	//typedef tbb::concurrent_unordered_map<HMF::Coordinate::HICANNGlobal, std::vector<BusSegment> > Segments;
	typedef std::map<HMF::Coordinate::HICANNGlobal, std::vector<BusSegment> > Segments;

	/// projcetion type
	typedef HardwareProjection Projection;

	// Constructor
	Route(BusSegment const source, std::vector<Projection> const& projections);

#ifndef PYPLUSPLUS
	BusSegment&       source();
#endif // PYPLUSPLUS
	BusSegment const& source() const;

	/// access the list of edges actually representing the route
	Segments const& getSegments() const;
	void setSegments(Segments const& s);
#ifndef PYPLUSPLUS
	void setSegments(Segments&& s);
#endif // PYPLUSPLUS

	/// the length is given by number of edges in the routing graph
	size_t length() const;
	bool empty() const;

	size_t numSources() const;

	void check(routing_graph const& g) const;

	/// get projections belonging to this connection
#ifndef PYPLUSPLUS
	std::vector<Projection>&       projections();
#endif // PYPLUSPLUS
	std::vector<Projection> const& projections() const;

private:
	/// Default constructor necessary for serialization (but private)
	Route();

	friend class LocalRoute;
	friend class boost::serialization::access;
	template<typename Archiver>
	void serialize(Archiver& ar, unsigned int const /*version*/);

	// members
	size_t mLength;

	/// Global index for the "entry point" into the routing graph
	BusSegment mSource;

	/// target HICANNs are mapped to their corresponding / predecessor list
	/// leading a way through the routing graph. One can get / from target to
	/// source by iterating over vector[0], vector[1], / ..., vector[n] with
	/// vector[0] == `target_vertical_bus`.
	boost::shared_ptr<Segments> mSegments;
	//Segments mSegments;

	/// list of projections. Necessary to make synapse lookups on the target
	/// side
	boost::shared_ptr< std::vector<Projection> > mProjections;
};

} // namespace routing
} // namespace marocco

namespace marocco {
namespace routing {

template<typename Archiver>
void Route::serialize(
	Archiver& ar, unsigned int const /*version*/)
{
	using namespace boost::serialization;
	ar & make_nvp("length", mLength)
	   & make_nvp("source", mSource)
	   & make_nvp("segments", mSegments)
	   & make_nvp("projections", mProjections);
}

} // namespace routing
} // namespace marocco
