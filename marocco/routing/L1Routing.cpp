#include "marocco/routing/L1Routing.h"

namespace marocco {
namespace routing {

namespace {

L1Route::segment_type to_segment(L1BusOnWafer const& bus)
{
	if (bus.is_horizontal()) {
		return bus.toHLineOnHICANN();
	}
	return bus.toVLineOnHICANN();
}

} // namespace

L1Route toL1Route(PathBundle::graph_type const& graph, PathBundle::path_type const& path)
{
	assert(!path.empty());
	auto const& bus = graph[path.front()];
	auto current_hicann = bus.toHICANNOnWafer();

	L1Route route;
	route.append(current_hicann, to_segment(bus));

	for (auto it = std::next(path.begin()), eit = path.end(); it != eit; ++it) {
		auto const& bus = graph[*it];
		auto hicann = bus.toHICANNOnWafer();
		if (hicann != current_hicann) {
			route.append(hicann, to_segment(bus));
			current_hicann = hicann;
		} else {
			route.append(to_segment(bus));
		}
	}

	return route;
}

L1RouteTree toL1RouteTree(PathBundle::graph_type const& graph, PathBundle const& bundle)
{
	L1RouteTree tree;
	for (auto const& path : bundle.paths()) {
		tree.add(toL1Route(graph, path));
	}
	return tree;
}

} // namespace routing
} // namespace marocco
