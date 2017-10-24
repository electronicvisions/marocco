#include "alone.h"

#include "hal/Coordinate/iter_all.h"
#include "marocco/routing/L1DijkstraRouter.h"
#include "marocco/routing/L1Routing.h"

using namespace HMF::Coordinate;

namespace marocco {
namespace alone {

Alone::Alone() : m_routing_graph()
{
}


void Alone::add(HMF::Coordinate::HICANNOnWafer const& hicann)
{
	m_routing_graph.add(hicann);
}

void Alone::remove(
    HMF::Coordinate::HICANNOnWafer const& hicann, HMF::Coordinate::HLineOnHICANN const& hline)
{
	m_routing_graph.remove(hicann, hline);
}

void Alone::remove(
    HMF::Coordinate::HICANNOnWafer const& hicann, HMF::Coordinate::VLineOnHICANN const& vline)
{
	m_routing_graph.remove(hicann, vline);
}

void Alone::remove(
    HMF::Coordinate::HICANNOnWafer const& hicann, HMF::Coordinate::HRepeaterOnHICANN const& hrep)
{
	m_routing_graph.remove(hicann, hrep);
}

void Alone::remove(
    HMF::Coordinate::HICANNOnWafer const& hicann, HMF::Coordinate::VRepeaterOnHICANN const& vrep)
{
	m_routing_graph.remove(hicann, vrep);
}

std::vector<L1Route> Alone::find_routes(
	routing::L1BusOnWafer const& source,
	routing::Target const& target,
	Options const options)
{
	auto const& graph = m_routing_graph.graph();
	routing::L1EdgeWeights weights(graph);
	routing::L1DijkstraRouter dijkstra(
	    weights, m_routing_graph[source],
	    (options & SWITCH_EXCLUSIVENESS_PER_ROUTE)
	        ? routing::L1DijkstraRouter::SwitchExclusiveness::per_route
	        : routing::L1DijkstraRouter::SwitchExclusiveness::global);
	dijkstra.add_target(target);
	dijkstra.run();
	std::vector<L1Route> routes;
	for (auto const& vertex : dijkstra.vertices_for(target)) {
		auto path = dijkstra.path_to(vertex);
		auto route = toL1Route(m_routing_graph.graph(), path);

		if (options & CONNECT_TEST_DATA_OUTPUT) {
			L1Route prefix;

			if (source.is_horizontal()) {
				prefix = {source.toHICANNOnWafer(),
						  source.toHLineOnHICANN().toHRepeaterOnHICANN().toRepeaterBlockOnHICANN()};
			} else {
				prefix = {source.toHICANNOnWafer(),
						  source.toVLineOnHICANN().toVRepeaterOnHICANN().toRepeaterBlockOnHICANN()};
			}

			route.prepend(prefix, L1Route::extend_mode::extend);
		}

		routes.push_back(route);
	}

	return routes;
}

std::vector<L1Route> Alone::find_routes(
    HMF::Coordinate::HICANNOnWafer const& hicann,
    HMF::Coordinate::DNCMergerOnHICANN const& merger,
    routing::Target const& target,
	Options const options)
{
	if (options & CONNECT_TEST_DATA_OUTPUT) {
		throw std::invalid_argument(
		    "can not connect test data output for route starting from DNC merger");
	}

	auto routes = find_routes(
	    {hicann, merger.toSendingRepeaterOnHICANN().toHLineOnHICANN()}, target, options);
	L1Route dnc_prefix{hicann, merger};
	for (auto& route : routes) {
		if (route.size() > 2) {
			// If we immediately switch from the source HICANN to the left, the intention is
			// to set the output mode of the sending repeater to LEFT.  This is encoded in a
			// L1Route by a DNCMergerOnHICANN directly followed by a HICANNOnWafer.
			auto it = route.begin();
			std::advance(it, 2);
			if (auto const* next_hicann = boost::get<HICANNOnWafer>(&*it)) {
				try {
					if (*next_hicann == hicann.west()) {
						// Strip the leading [HICANNOnWafer, HLineOnWafer] segments.
						route = route.split(it).second;
					}
				} catch (std::overflow_error const&) {
					// reached bound of wafer, other HICANN does not exist
				} catch (std::domain_error const&) {
					// invalid combination of X and Y (can happen because wafer is round)
				}
			}
		}

		route.prepend(dnc_prefix, L1Route::extend_mode::extend);
	}
	return routes;
}

} // namespace alone
} // namespace marocco
