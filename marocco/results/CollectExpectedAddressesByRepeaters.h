#pragma once

#include <map>
#include <vector>

#include <boost/variant/apply_visitor.hpp>
#include <boost/variant/multivisitors.hpp>
#include <boost/variant/static_visitor.hpp>
#include <boost/variant/variant.hpp>

#include "marocco/coordinates/L1Route.h"
#include "marocco/results/Marocco.h"

namespace marocco {
namespace results {

struct CollectExpectedAddressesByRepeaters : public boost::static_visitor<>
{
	typedef std::vector<HMF::HICANN::L1Address> addresses_type;
	typedef boost::variant<halco::hicann::v2::HRepeaterOnHICANN, halco::hicann::v2::VRepeaterOnHICANN>
		repeater_type;

	/// \brief Expected source addresses of all possible events fed into the current route.
	/// These are extracted from the results container and used as the expected addresses
	/// of individual repeaters when traversing the route in the visitor.
	addresses_type source_addresses_of_current_route;

	/// \brief Expected event addresses for every repeater actively involved in a L1 route
	///        of the network.
	std::map<halco::hicann::v2::HICANNOnWafer, std::map<repeater_type, addresses_type> >
		expected_addresses;

	/// \brief Expected event addresses for repeaters adjacent to L1 routes of the network.
	/// These repeaters constitute possible additional points where events can be extracted
	/// using test ports.  For example an unused repeater on an adjacent HICANN that is
	/// physically connected to a L1 bus used by the network but not actively involved in
	/// any L1 routes.
	/// \note This map is "shadowed" by the repeaters actively involved in the network.
	///       Consequently a repeater should only be considered free-to-use if it is not
	///       contained in \c expected_addresses.  E.g. a horizontal repeater may have
	///       been added to \c passive_addresses as it is adjacent to a L1 route, but is
	///       later also used as the sending repeater for a different route and thus end
	///       up in \c expected_addresses.
	/// \note For testing these repeaters have to be set in forwarding mode and the sending
	//        direction has to be set accordingly. Furthermore, one has to make sure that
	//        the forwarding of addresses by these passive repeaters does not affect other
	//        parts of the route, e.g. by forwarding addresses in used lines.
	std::map<halco::hicann::v2::HICANNOnWafer, std::map<repeater_type, addresses_type> >
		passive_addresses;
	halco::hicann::v2::HICANNOnWafer current_hicann;

	void apply(marocco::results::Marocco const& results)
	{
		for (auto const& route_item : results.l1_routing) {
			source_addresses_of_current_route.clear();

			for (auto const& item : results.placement.find(route_item.source())) {
				auto const& address = item.address();
				assert(address != boost::none);
				source_addresses_of_current_route.push_back(address->toL1Address());
			}

			auto const& route = route_item.route();
			apply(route.begin(), route.end());
		}
	}

	void apply(marocco::L1Route::iterator it, marocco::L1Route::iterator const end)
	{
		auto next = std::next(it);

		while (next != end) {
			if (auto const* hicann = boost::get<halco::hicann::v2::HICANNOnWafer>(&*it)) {
				current_hicann = *hicann;
			}

			boost::apply_visitor(*this, *it, *next);

			auto next_next = std::next(next);
			if (next_next == end)
				break;

			boost::apply_visitor(*this, *it, *next, *next_next);

			it = next;
			next = next_next;
		}
	}

	template <typename T, typename V>
	void operator()(T const&, V const&)
	{}

	void operator()(
		halco::hicann::v2::HLineOnHICANN const& current, halco::hicann::v2::VLineOnHICANN const& next)
	{
		passive_addresses[current_hicann][current.toHRepeaterOnHICANN()] =
			source_addresses_of_current_route;
		passive_addresses[current_hicann][next.toVRepeaterOnHICANN()] =
			source_addresses_of_current_route;
	}

	void operator()(
		halco::hicann::v2::VLineOnHICANN const& current, halco::hicann::v2::HLineOnHICANN const& next)
	{
		passive_addresses[current_hicann][current.toVRepeaterOnHICANN()] =
			source_addresses_of_current_route;
		passive_addresses[current_hicann][next.toHRepeaterOnHICANN()] =
			source_addresses_of_current_route;
	}

	template <typename T, typename U, typename V>
	void operator()(T const&, U const&, V const&)
	{}

	void operator()(
		halco::hicann::v2::HLineOnHICANN const& current,
		halco::hicann::v2::HICANNOnWafer hicann,
		halco::hicann::v2::HLineOnHICANN const& next)
	{
		auto repeater = current.toHRepeaterOnHICANN();
		if (current.east() == next && repeater.isRight()) {
			// Repeater on current HICANN is used for route.
			hicann = hicann.west();
			assert(hicann == current_hicann);
		} else if (current.west() == next && repeater.isLeft()) {
			// Repeater on current HICANN is used for route.
			hicann = hicann.east();
			assert(hicann == current_hicann);
		} else {
			// Repeater on current HICANN does not actively participate in route.
			passive_addresses[current_hicann][repeater] = source_addresses_of_current_route;
			// Repeater on next HICANN is used for route.
			repeater = next.toHRepeaterOnHICANN();
		}
		expected_addresses[hicann][repeater] = source_addresses_of_current_route;
	}

	void operator()(
		halco::hicann::v2::VLineOnHICANN const& current,
		halco::hicann::v2::HICANNOnWafer hicann,
		halco::hicann::v2::VLineOnHICANN const& next)
	{
		auto repeater = current.toVRepeaterOnHICANN();
		if (current.south() == next && repeater.isBottom()) {
			// Repeater on current HICANN is used for route.
			hicann = hicann.north();
			assert(hicann == current_hicann);
		} else if (current.north() == next && repeater.isTop()) {
			// Repeater on current HICANN is used for route.
			hicann = hicann.south();
			assert(hicann == current_hicann);
		} else {
			// Repeater on current HICANN does not actively participate in route.
			passive_addresses[current_hicann][repeater] = source_addresses_of_current_route;
			// Repeater on next HICANN is used for route.
			repeater = next.toVRepeaterOnHICANN();
		}
		expected_addresses[hicann][repeater] = source_addresses_of_current_route;
	}

	// "output to the left" case of sending repeater
	void operator()(
		halco::hicann::v2::DNCMergerOnHICANN const& current,
		halco::hicann::v2::HICANNOnWafer const& hicann,
		halco::hicann::v2::HLineOnHICANN const& /*next*/)
	{
		auto const repeater = current.toSendingRepeaterOnHICANN().toHRepeaterOnHICANN();
		expected_addresses[hicann.east()][repeater] = source_addresses_of_current_route;
	}

	// "output to the right" case of sending repeater
	void operator()(
		halco::hicann::v2::HICANNOnWafer const& hicann,
		halco::hicann::v2::DNCMergerOnHICANN const& current,
		halco::hicann::v2::HLineOnHICANN const& /*next*/)
	{
		auto const repeater = current.toSendingRepeaterOnHICANN().toHRepeaterOnHICANN();
		expected_addresses[hicann][repeater] = source_addresses_of_current_route;
	}
}; // CollectExpectedAddressesByRepeaters

} // namespace results
} // namespace marocco
