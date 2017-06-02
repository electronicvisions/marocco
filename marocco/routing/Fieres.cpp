#include <algorithm>

#include "hal/Coordinate/Quadrant.h"
#include "hal/Coordinate/iter_all.h"
#include "marocco/Logger.h"
#include "marocco/routing/Fieres.h"
#include "marocco/util/algorithm.h"

using namespace HMF::Coordinate;

namespace marocco {
namespace routing {

namespace fieres {

const int InboundRoute::DEFECT = -1;

void Assignment::add_defect(SideVertical const& side, coordinate_type const& drv)
{
	auto& ptr = mData[side][drv];
	if (ptr) {
		throw std::runtime_error("Synapse driver already taken.");
	}
	ptr = std::make_shared<interval_type>();
}

void Assignment::insert(
	SideVertical const& side_vertical, coordinate_type const& drv, InboundRoute const& route)
{
	size_t const length = route.assigned;

	if (length < 1) {
		return;
	}

	auto& assign = mData[side_vertical];
	auto const begin_it = assign.begin();
	auto const primary_it = begin_it + drv;
	array_type::reverse_iterator const primary_rit{primary_it};

	if (*primary_it) {
		throw std::runtime_error("Primary synapse driver already taken.");
	}

	// Look for up to (length - 1) free spots above, excluding primary:
	auto const top_it =
		std::find_if(primary_rit, std::min(primary_rit + (length - 1), assign.rend()), assigned_p)
			.base();
	auto const possible_above = primary_it - top_it;

	// Look for the remaining spots below, including primary:
	// (Note that bottom points past the last element, like .end().)
	auto const bottom_it = std::find_if(
		primary_it, std::min(primary_it + (length - possible_above), assign.end()), assigned_p);

	if (size_t(bottom_it - top_it) != length) {
		throw std::runtime_error("Could not fit assignment range into gap.");
	}

	auto const ival =
		std::make_shared<interval_type>(route, drv, top_it - begin_it, bottom_it - begin_it);
	std::fill(top_it, bottom_it, ival);
}

namespace {

/// Auxiliary structure to keep track of possible insertion options
struct Option {
	    Option(SynapseDriverOnQuadrant drv, SideVertical side, size_t n)
	        : primary(std::move(drv)),
	          side_vertical(std::move(side)),
	          gap(std::move(n)) {}

	SynapseDriverOnQuadrant primary;
	SideVertical side_vertical;
	/// Number of unassigned slots around primary driver.
	size_t gap;

	friend bool operator<(Option const& lhs, Option const& rhs) {
		if (lhs.gap != rhs.gap) {
			return lhs.gap < rhs.gap;
		} else if (lhs.side_vertical != rhs.side_vertical) {
			return lhs.side_vertical < rhs.side_vertical;
		} else {
			return lhs.primary < rhs.primary;
		}
	}
};

} // namespace

bool Assignment::add(InboundRoute const& route) {
	// We look at all synapse drivers reachable from the
	// VLineOnHICANN of this route.  We can then connect unused
	// adjacent synapse drivers to this "primary" driver to arrive
	// at the desired number of synapse drivers.

	auto const& drivers = route.line.toSynapseDriverOnHICANN(mSide);
	size_t const length = route.assigned;
	std::vector<Option> options;

	for (auto const& syndrv : drivers) {
		auto const side_vertical = syndrv.toSideVertical();
		auto const primary = syndrv.toSynapseDriverOnQuadrant();
		auto const& assign = mData[side_vertical];
		auto const it = assign.begin() + primary;

		if (*it) {
			// This synapse driver is already taken.
			continue;
		}

		// Count "gap", i.e. number of unassigned drivers surrounding the
		// given primary driver:
		array_type::const_reverse_iterator const rit{it};

		size_t const gap = (
			// look below: [it, assign.end())
			algorithm::count_while(it, assign.end(), unassigned_p) +
			// look above: [assign.begin(), it) <=> (it, assign.rend())
			algorithm::count_while(rit, assign.rend(), unassigned_p));

		options.emplace_back(primary, side_vertical, gap);

		// Abort if we happen to have a perfect match.
		if (gap == length) {
			break;
		}
	}

	if (options.empty()) {
		// None of the reachable synapse drivers is free.
		return false;
	}

	// Sort possible insertion points by increasing number of
	// unassigned drivers (gap).
	std::sort(options.begin(), options.end());

	// Pick the smallest possible (but still matching) gap.
	// If there are two equally good options the one with
	// the smaller primary driver will be prefered because of the sorting.
	auto choice = std::lower_bound(
		options.cbegin(), options.cend(), length,
		[](Option const& opt, size_t value) {
			return opt.gap < value;
		});

	if (choice != options.cend()) {
		// We found the smallest option with gap >= length.
		insert(choice->side_vertical, choice->primary, route);
	} else {
		// There is no large enough gap for this incoming route.
		// Pick the next best one as a last resort.
		auto const& last = options.back();
		InboundRoute possible(route.line, route.drivers, route.synapses, last.gap);
		insert(last.side_vertical, last.primary, possible);
	}

	return true;
}

auto Assignment::result() const -> result_type
{
	result_type res;
	std::unordered_set<interval_type const*> processed;

	for (auto const side_vertical : iter_all<SideVertical>()) {
		for (auto const& interval : mData[side_vertical]) {
			if (!interval || interval->route.drivers == InboundRoute::DEFECT ||
			    !processed.insert(interval.get()).second) {
			    // No interval, defect driver or interval already processed.
				continue;
			}

			QuadrantOnHICANN quadrant{side_vertical, mSide};
			results::ConnectedSynapseDrivers drivers(
				interval->primary.toSynapseDriverOnHICANN(quadrant));

			drivers.connect(SynapseDriverOnQuadrant(interval->begin));
			drivers.connect(SynapseDriverOnQuadrant(interval->end - 1));

			res[interval->route.line].push_back(drivers);
		}
	}

	return res;
}

} // namespace fieres


namespace {

/// function that does the actual bin packing. Looks for suitable insertion
/// points for entries of @param list in @param assignment.
void defrag(std::list<fieres::InboundRoute> const& list, fieres::Assignment& assignment,
            std::vector<VLineOnHICANN>& rejected) {
	// We want to favor inbound routes with many synapses, thus we sort
	// by descending number of synapses.

	std::vector<fieres::InboundRoute> sorted;
	sorted.reserve(list.size());
	std::copy(list.begin(), list.end(), std::back_inserter(sorted));
	std::sort(sorted.begin(), sorted.end(), [](fieres::InboundRoute const& lhs, fieres::InboundRoute const& rhs) {
		return lhs.synapses > rhs.synapses;
	});

	for (auto const& val : sorted) {
		if (!assignment.add(val)) {
			rejected.push_back(val.line);
		}
	} // for all assignments
}

} // namespace

Fieres::Fieres(IntervalList const& _list,
			   HMF::Coordinate::Side const& side,
			   size_t max_chain_length) :
	// call delegate constructor with no defects
	Fieres(_list, side, max_chain_length, std::vector<SynapseDriverOnHICANN>{})
{}


Fieres::Fieres(IntervalList const& _list,
               HMF::Coordinate::Side const& side,
               size_t const max_chain_length,
               std::vector<SynapseDriverOnHICANN> const& defects)
	: mSide(side)
{
	// Count number of requested drivers and synapses
	size_t const synapse_count = std::accumulate(
		_list.begin(), _list.end(), 0, [](size_t cnt, DriverInterval const& entry) {
			return cnt + entry.synapses;
		});

	size_t const driver_count = std::accumulate(
		_list.begin(), _list.end(), 0, [](size_t cnt, DriverInterval const& entry) {
			return cnt + entry.driver;
		});

	MAROCCO_DEBUG("Requested Driver Intervals:");

	for (auto di : _list) {
		MAROCCO_DEBUG(di);
	}

	MAROCCO_DEBUG("Synapse Drivers required: " << driver_count );

	// Count number of available drivers: 112 - defect drivers on this side
	auto const _side = mSide; // copy of side, which can be captured by lambda
	auto isMySide = [_side] ( SynapseDriverOnHICANN const & driver) -> bool {
		return driver.toSideHorizontal() == _side;
	};
	size_t const driver_available = SynapseDriverOnQuadrant::end*QuadrantOnHICANN::y_type::end
						- std::count_if(defects.begin(), defects.end(), isMySide);

	MAROCCO_DEBUG("Synapse Drivers available: " << driver_available);

	typedef std::list<fieres::InboundRoute> List;
	List list;
	List last_resort;

	////////////////////////////////////////////////////////
	// Less requested than available:                     //
	// assign considering the max chain length constraint //
	////////////////////////////////////////////////////////

	if ( driver_count <= driver_available ) {
		for (DriverInterval const& entry : _list)
		{
			double const syns = entry.synapses;
			size_t const assign = std::min(max_chain_length, entry.driver);
			if (assign==0 || syns==0.) {
				throw std::runtime_error("assignment error");
			}
			list.insert(list.end(), fieres::InboundRoute(entry.line, assign, syns, assign));
		}
	}

	//////////////////////////////////////////////////////////////////////////////
	// Less available than requested:                                           //
	// Normalize according to the fraction of total synapses realized per input //
	// See Jeltsch 2014, Eq. 6.10, etc.                                         //
	//////////////////////////////////////////////////////////////////////////////

	else {

		std::multimap<double, List::iterator, std::greater<double>> too_many;
		std::multimap<double, List::iterator, std::greater<double>> too_few;
		size_t assigned = 0;
		bool rescale = false;

		// First, construct a list of pending assignments for subsequent bin
		// packing.
		for (DriverInterval const& entry : _list)
		{
			double const syns = entry.synapses;
			size_t const assign = std::min(max_chain_length, std::max<size_t>(1, std::min<size_t>(entry.driver, 1.*syns/synapse_count*driver_available)));
			rescale |= (entry.driver != assign);
			assigned+=assign;
			if (assign==0 || syns==0.) {
				throw std::runtime_error("assignment error");
			}

			auto it = list.insert(list.end(), fieres::InboundRoute(entry.line, std::min(max_chain_length, entry.driver), syns, assign));
			double const _delta = double(assign) - 1.*syns/synapse_count*driver_available;
			if (_delta>0.) {
				too_many.insert(std::make_pair(_delta, it));
			} else if(_delta<0. && assign<entry.driver) {
				too_few.insert(std::make_pair(std::abs(_delta), it));
			}
		}

		MAROCCO_DEBUG("Synapse Drivers initially assigned: " << assigned << ". Need rescale? " << rescale );

		if (assigned!=driver_available && rescale)
		{
			while(assigned<driver_available)
			{
				// increase assignments, start with biggest delta first
				bool change = false;
				for (auto it=too_few.begin(); it!=too_few.end() && assigned<driver_available; ++it)
				{
					fieres::InboundRoute& val = *(it->second);
					if (static_cast<int>(val.assigned) < val.drivers) {
						val.assigned++;
						assigned++;
						change = true;
					}

					//if (val.assigned==val.drivers) {
						//too_few.erase(it);
					//}

					if (assigned==driver_available) {
						break;
					}
				}

				if (!change) {
					break;
				}
			}


			// In the following, the normalization artefacts are corrected if either too many or too few
			// synapse drivers have ben assigned.


			// decrease assignments, start with smallest delta first
			while (assigned>driver_available)
			{
				for (auto it=too_many.begin(); it!=too_many.end() && assigned>driver_available; ++it)
				{
					fieres::InboundRoute& val = *(it->second);
					if (val.assigned>0) {
						val.assigned--;
						assigned--;

						if (val.assigned<=0) {
							last_resort.push_back(val);
							list.erase(it->second);
							//too_many.erase(it);
						}
					}

					if (assigned<=driver_available) {
						break;
					}
				}
			}

			size_t const debug = std::accumulate(
				list.begin(), list.end(), 0, [](size_t acc, fieres::InboundRoute const& entry) {
					return acc + entry.assigned;
				});
			if (assigned != debug) {
				throw std::runtime_error("broken resize");
			}
		}
	} // end of: Less available than requested


	// do the defragmentation
	fieres::Assignment assignment(mSide);
	for (auto const& entry : defects)
	{
		if (entry.toSideHorizontal()==mSide) {
			// allocate defect entries, to avoid assignment later on
			assignment.add_defect(entry.toSideVertical(), entry.toSynapseDriverOnQuadrant());
		}
	}

	// lock defect synapse drivers first.

	size_t const assigned_after_rescale = std::accumulate(
		list.begin(), list.end(), 0, [](size_t cnt, fieres::InboundRoute const& entry) {
			return cnt + entry.assigned;
		});

	MAROCCO_DEBUG("Synapse Drivers assigned after rescale: " << assigned_after_rescale);

	defrag(list, assignment, mRejected);

	// we could also take another insertion point for allready inserted driver
	for (auto& entry : last_resort)
	{
		entry.assigned = entry.drivers;
	}
	defrag(last_resort, assignment, mRejected);

	// build result
	mResult = assignment.result();
}

Fieres::Result Fieres::result() const
{
	return mResult;
}

Fieres::Rejected Fieres::rejected() const
{
	return mRejected;
}

} // namespace routing
} // namespace marocco
