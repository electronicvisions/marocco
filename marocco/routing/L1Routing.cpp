#include "marocco/routing/L1Routing.h"

using namespace HMF::Coordinate;

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

L1Route with_dnc_merger_prefix(L1Route const& route, DNCMergerOnWafer const& merger)
{
	if (route.empty()) {
		return route;
	}
	assert(route.size() >= 2); // Non-empty routes have at least two elements.

	// Operate on copy, so original route is not modified if the prepend operation throws.
	L1Route result = route;
	L1Route const dnc_prefix{merger.toHICANNOnWafer(), merger.toDNCMergerOnHICANN()};
	HICANNOnWafer const hicann = route.source_hicann();

	// If we immediately switch from the source HICANN to the left, the intention is
	// to set the output mode of the sending repeater to LEFT.  This is encoded in a
	// L1Route by a DNCMergerOnHICANN directly followed by a HICANNOnWafer.
	auto it = result.begin();
	std::advance(it, 2);
	if (auto const* next_hicann = boost::get<HICANNOnWafer>(&*it)) {
		try {
			if (*next_hicann == hicann.west()) {
				// Strip the leading [HICANNOnWafer, HLineOnWafer] segments.
				result = result.split(it).second;
			}
		} catch (std::overflow_error const&) {
			// reached bound of wafer, other HICANN does not exist
		} catch (std::domain_error const&) {
			// invalid combination of X and Y (can happen because wafer is round)
		}
	}

	result.prepend(dnc_prefix, L1Route::extend_mode::extend);

	return result;
}

} // namespace routing
} // namespace marocco
