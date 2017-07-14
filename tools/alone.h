#pragma once

#include <vector>

#include "sthal/Wafer.h"
#include "marocco/coordinates/L1Route.h"
#include "marocco/routing/Configuration.h"
#include "marocco/routing/L1BusOnWafer.h"
#include "marocco/routing/Target.h"

#ifndef PYPLUSPLUS
#include "marocco/routing/L1RoutingGraph.h"
#else
namespace marocco {
namespace routing {
class L1RoutingGraph
{
};
} // namespace routing
} // namespace marocco
#endif // !PYPLUSPLUS

namespace marocco {
namespace alone {

class Alone
{
public:
	Alone();

	void add(HMF::Coordinate::HICANNOnWafer const& hicann);
	void remove(
		HMF::Coordinate::HICANNOnWafer const& hicann,
		HMF::Coordinate::HLineOnHICANN const& hline);
	void remove(
		HMF::Coordinate::HICANNOnWafer const& hicann,
		HMF::Coordinate::VLineOnHICANN const& vline);
	void remove(
		HMF::Coordinate::HICANNOnWafer const& hicann,
		HMF::Coordinate::HRepeaterOnHICANN const& hrep);
	void remove(
		HMF::Coordinate::HICANNOnWafer const& hicann,
		HMF::Coordinate::VRepeaterOnHICANN const& vrep);

	/**
	 * @brief Return all possible routes from the given bus to a bus matching
	 *        the target specification.
	 * @note The returned routes only consist of \c HLineOnHICANN and
	 *       \c VLineOnHICANN segments.  If you want a \c L1Route suitable for
	 *       the configuration of sending repeaters, use the overload of this
	 *       function that allows passing a \c DNCMergerOnHICANN as source.
	 * @param connect_test_data_output Whether test data output from the repeater block
	 *        should be sent to the originating bus.
	 */
	std::vector<L1Route> find_routes(
		routing::L1BusOnWafer const& source, routing::Target const& target,
		bool connect_test_data_output = false);

	/**
	 * @brief Return all possible routes from the given DNC merger to a bus
	 *        matching the target specification.
	 * @note The returned routes will include \c DNCMergerOnHICANN segments
	 *       necessary to correctly configure the sending repeaters.
	 */
	std::vector<L1Route> find_routes(
	    HMF::Coordinate::HICANNOnWafer const& hicann,
	    HMF::Coordinate::DNCMergerOnHICANN const& merger,
	    routing::Target const& target);

private:
	routing::L1RoutingGraph m_routing_graph;
}; // Alone

} // namespace alone
} // namespace marocco
