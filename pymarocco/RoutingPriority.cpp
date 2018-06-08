#include "pymarocco/RoutingPriority.h"
#include <stdexcept>
#include <utility>

namespace pymarocco {

RoutingPriority::RoutingPriority() :
	mPriorities()
{}

void RoutingPriority::prioritize(ProjectionId proj, size_t weight)
{
	if (!mPriorities.insert(std::make_pair(proj, weight)).second)
	{
		throw std::runtime_error("Projection already prioritized.");
	}
}

size_t RoutingPriority::get(ProjectionId proj) const
{
	auto it = mPriorities.find(proj);
	return (it != mPriorities.end() ? it->second : 0);
}

} // pymarocco
