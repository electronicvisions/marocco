#include "marocco/routing/LocalRoute.h"
#include "marocco/routing/Route.h"

namespace marocco {
namespace routing {

LocalRoute::LocalRoute(Route const& route, Route::BusSegment const& bus):
	mBus(bus),
	mRoute(route)
{
	mNumSources = mRoute.numSources();

	// TODO: check mBus is actually target bus of route referenced by mRoute
}

size_t LocalRoute::numSources() const
{
	return mNumSources;
}

L1Bus const& LocalRoute::targetBus(routing_graph const& rgraph) const
{
	L1Bus const& bus = rgraph[mBus];
	assert(bus.getDirection() == L1Bus::Vertical);
	return bus;
}

Route::BusSegment LocalRoute::targetBus() const
{
	return mBus;
}

Route const& LocalRoute::route() const
{
	return mRoute;
}

int LocalRoute::id() const
{
	return mBus;
}

LocalRoute::LocalRoute()
{}

} // namespace routing
} // namespace marocco
