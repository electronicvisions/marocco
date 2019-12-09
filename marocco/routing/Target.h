#pragma once

#include <boost/operators.hpp>

#include "hal/Coordinate/HICANN.h"
#include "hal/Coordinate/Relations.h"

namespace marocco {
namespace routing {

class Target : boost::operators<Target>
{
public:
	Target(
	    HMF::Coordinate::HICANNOnWafer const& hicann,
	    HMF::Coordinate::Orientation const& orientation);

	HMF::Coordinate::HICANNOnWafer toHICANNOnWafer() const;
	HMF::Coordinate::Orientation toOrientation() const;

	bool operator==(Target const& other) const;

private:
	HMF::Coordinate::HICANNOnWafer m_hicann;
	HMF::Coordinate::Orientation m_orientation;
}; // Target

} // namespace routing
} // namespace marocco

namespace std {

template <>
struct hash<marocco::routing::Target>
{
	size_t operator()(marocco::routing::Target const& target) const
	{
		size_t hash = 0;
		boost::hash_combine(hash, target.toHICANNOnWafer().toEnum());
		boost::hash_combine(hash, target.toOrientation());
		return hash;
	}
};

} // namespace std
