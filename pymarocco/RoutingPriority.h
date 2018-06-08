#pragma once

#include <map>
#include <boost/serialization/nvp.hpp>
#include "hal/Coordinate/HMFGeometry.h"

namespace pymarocco {

class RoutingPriority
{
public:
	typedef size_t ProjectionId;

	RoutingPriority();

	void prioritize(ProjectionId proj, size_t weight);
	size_t get(ProjectionId proj) const;

private:
	std::map<ProjectionId, size_t> mPriorities;

	friend class boost::serialization::access;
	template <typename Archive>
	void serialize(Archive& ar, unsigned int const)
	{
		using boost::serialization::make_nvp;
		ar& make_nvp("priority", mPriorities);
	}
};

} // pymarocco
