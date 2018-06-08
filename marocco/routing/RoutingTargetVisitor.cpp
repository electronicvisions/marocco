#include "marocco/routing/RoutingTargetVisitor.h"
#include "marocco/Logger.h"

namespace marocco {
namespace routing {

RoutingTargetVisitor::RoutingTargetVisitor(
	std::vector<int> const& predecessor,
	usage_t& usage,
	std::unordered_set<target_t>& targets,
	last_mile_t& lastMile) :
		mUsage(usage),
		mPredecessor(predecessor),
		mTargets(targets),
		mLastMile(lastMile)
{
	assert(lastMile.empty());
	if (mTargets.empty()) {
		throw RoutingEarlyAbort("no targets");
	}
}

} // namespace routing
} // namespace marocco
