#pragma once

#include <boost/property_map/property_map.hpp>
#include <array>
#include "marocco/routing/routing_graph.h"
#include "marocco/placement/Result.h"
#include "pymarocco/Routing.h"

namespace marocco {
namespace routing {

/**
 * Weight map for the WaferRouting graph (remember: vertices are L1 busses and
 * edges between these verties represent the connectivity between L1 buses).
 * This map returns an edge weight of 1 for most connections between L1 busses,
 * except those connecting SendingRepeaters. In this case a weight of 10000 is
 * returned.
 */
class WeightMap :
	public boost::put_get_helper<int, WeightMap>
{
public:
	typedef routing_graph::edge_descriptor key_type;
	typedef int value_type;
	typedef int reference;
    typedef boost::readable_property_map_tag category;

	WeightMap(
		routing_graph const& rg,
		placement::internal::WaferL1AddressAssignment const& address_assignment);
	WeightMap(
		pymarocco::Routing const& param,
		routing_graph const& rg,
		placement::internal::WaferL1AddressAssignment const& address_assignment);

	reference operator[] (key_type const& k) const;

	// current bus utilization, constantly updated by Visitor.

	/// bus utilization for horizontal busses
	std::array<std::uint16_t, 384> horizontal;

	/// bus utilization for vertical busses
	std::array<std::uint16_t, 384> vertical;

private:
	routing_graph const& g;

	size_t const w_vertical = 5;
	size_t const w_horizontal = 8;
	size_t const w_SPL1 = 8; // TODO: remove me, no longer needed
	size_t const w_straight_horizontal = 5; // TODO: remove me, no longer needed
	size_t const w_straight_vertical = 2; // TODO: remove me, no longer needed
	size_t const w_congestion_factor = 1;

	placement::internal::WaferL1AddressAssignment const& mAddressAssignment;
};

} // namespace routing
} // namespace marocco
