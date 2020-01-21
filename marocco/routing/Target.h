#pragma once

#include <boost/operators.hpp>

#include "halco/hicann/v2/hicann.h"
#include "halco/common/relations.h"

namespace marocco {
namespace routing {

class Target : boost::operators<Target>
{
public:
	Target(
	    halco::hicann::v2::HICANNOnWafer const& hicann,
	    halco::common::Orientation const& orientation);

	halco::hicann::v2::HICANNOnWafer toHICANNOnWafer() const;
	halco::common::Orientation toOrientation() const;

	bool operator==(Target const& other) const;

private:
	halco::hicann::v2::HICANNOnWafer m_hicann;
	halco::common::Orientation m_orientation;
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
