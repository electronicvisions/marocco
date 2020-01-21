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

	enum Options
	{
		NONE = 0x0,
		CONNECT_TEST_DATA_OUTPUT = 0x1,
		SWITCH_EXCLUSIVENESS_PER_ROUTE = 0x2
	};

	void add(halco::hicann::v2::HICANNOnWafer const& hicann);
	void remove(
		halco::hicann::v2::HICANNOnWafer const& hicann,
		halco::hicann::v2::HLineOnHICANN const& hline);
	void remove(
		halco::hicann::v2::HICANNOnWafer const& hicann,
		halco::hicann::v2::VLineOnHICANN const& vline);
	void remove(
		halco::hicann::v2::HICANNOnWafer const& hicann,
		halco::hicann::v2::HRepeaterOnHICANN const& hrep);
	void remove(
		halco::hicann::v2::HICANNOnWafer const& hicann,
		halco::hicann::v2::VRepeaterOnHICANN const& vrep);

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
	    routing::L1BusOnWafer const& source, routing::Target const& target, Options options = NONE);

	/**
	 * @brief Return all possible routes from the given DNC merger to a bus
	 *        matching the target specification.
	 * @note The returned routes will include \c DNCMergerOnHICANN segments
	 *       necessary to correctly configure the sending repeaters.
	 */
	std::vector<L1Route> find_routes(
	    halco::hicann::v2::HICANNOnWafer const& hicann,
	    halco::hicann::v2::DNCMergerOnHICANN const& merger,
	    routing::Target const& target,
	    Options options = NONE);

private:
	routing::L1RoutingGraph m_routing_graph;
}; // Alone

} // namespace alone
} // namespace marocco
