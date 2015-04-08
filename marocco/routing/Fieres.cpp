#include <algorithm>

#include "hal/Coordinate/iter_all.h"
#include "marocco/Logger.h"
#include "marocco/alg.h"
#include "marocco/routing/Fieres.h"
#include "marocco/routing/util.h"

using namespace HMF::Coordinate;

namespace marocco {
namespace routing {

namespace fieres {

const int InboundRoute::DEFECT = -1;

void Assignment::add_defect(side_vertical_t const& side, coord_t const& drv) {
	auto& ptr = mData[side][drv];
	if (ptr) {
		throw std::runtime_error("Synapse driver already taken.");
	}
	ptr = std::make_shared<interval_t>();
}

void Assignment::insert(side_vertical_t const& side_vertical, coord_t const& drv,
                        InboundRoute const& route) {
	size_t const length = route.assigned;

	if (length < 1) {
		return;
	}

	auto& assign = mData[side_vertical];
	auto const begin = assign.begin();
	auto const primary = begin + drv;
	array_t::reverse_iterator rprimary{primary};

	if (*primary) {
		throw std::runtime_error("Primary synapse driver already taken.");
	}

	// Look for up to (length - 1) free spots above, excluding primary:
	auto const top =
	    std::find_if(rprimary, std::min(rprimary + (length - 1), assign.rend()),
	                 assigned_p).base();
	auto const possible_above = primary - top;

	// Look for the remaining spots below, including primary:
	// (Note that bottom points past the last element, like .end().)
	auto const bottom = std::find_if(
	    primary, std::min(primary + (length - possible_above), assign.end()), assigned_p);

	if (size_t(bottom - top) != length) {
		throw std::runtime_error("Could not fit assignment range into gap.");
	}

	auto ival = std::make_shared<interval_t>(route, drv, top - begin, bottom - begin);
	std::fill(top, bottom, ival);
}

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
		array_t::const_reverse_iterator const rit{it};

		size_t const gap = (
			// look below: [it, assign.end())
			alg::count_while(it, assign.end(), unassigned_p) +
			// look above: [assign.begin(), it) <=> (it, assign.rend())
			alg::count_while(rit, assign.rend(), unassigned_p));

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

Assignment::result_t Assignment::result() const {
	result_t res;
	std::unordered_set<interval_t const*> processed;

	for (auto const side_vertical : iter_all<SideVertical>()) {
		for (auto const& interval : mData[side_vertical]) {
			if (!interval || interval->route.drivers == InboundRoute::DEFECT ||
			    !processed.insert(interval.get()).second) {
			    // No interval, defect driver or interval already processed.
				continue;
			}

			DriverAssignment as;
			QuadrantOnHICANN quadrant{side_vertical, mSide};
			as.primary = interval->primary.toSynapseDriverOnHICANN(quadrant);

			for (size_t ii = interval->begin; ii < interval->end; ++ii) {
				as.drivers.insert(
				    SynapseDriverOnQuadrant(ii).toSynapseDriverOnHICANN(quadrant));
			}

			if (as.drivers.empty()) {
				throw std::runtime_error("empty drivers");
			}

			res[interval->route.line].push_back(as);
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


	typedef std::list<fieres::InboundRoute> List;
	List list;
	std::multimap<double, List::iterator, std::greater<double>> too_many;
	std::multimap<double, List::iterator, std::greater<double>> too_few;
	size_t assigned = 0;
	bool rescale = false;

	// First, construct a list of pending assignments for subsequent bin
	// packing.
	for (DriverInterval const& entry : _list)
	{
		double const syns = entry.synapses;
		size_t const assign = std::min(max_chain_length, std::max<size_t>(1, std::min<size_t>(entry.driver, syns/synapse_count*112.)));
		rescale |= (entry.driver != assign);
		assigned+=assign;
		if (assign==0 || syns==0.) {
			throw std::runtime_error("assignment error");
		}

		auto it = list.insert(list.end(), fieres::InboundRoute(entry.line, std::min(max_chain_length, entry.driver), syns, assign));
		// FIXME: shouldn't this be:
		//     double const _delta = double(entry.driver) - syns/synapse_count*112.;
		// rather than the following?
		double const _delta = double(assign) - syns/synapse_count*112.;
		if (_delta>0.) {
			too_many.insert(std::make_pair(_delta, it));
		} else if(_delta<0. && assign<entry.driver) {
			too_few.insert(std::make_pair(std::abs(_delta), it));
		}
	}

	MAROCCO_DEBUG("Synapse Drivers initially assigned: " << assigned << ". Need rescale? " << rescale );

	std::list<fieres::InboundRoute> last_resort;
	if (assigned!=112 && rescale)
	{
		while(assigned<112)
		{
			// increase assignments, start with biggest delta first
			bool change = false;
			for (auto it=too_few.begin(); it!=too_few.end() && assigned<112; ++it)
			{
				fieres::InboundRoute& val = *(it->second);
				if (val.assigned<val.drivers) {
					val.assigned++;
					assigned++;
					change = true;
				}

				//if (val.assigned==val.drivers) {
					//too_few.erase(it);
				//}

				if (assigned==112) {
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
		while (assigned>112)
		{
			for (auto it=too_many.begin(); it!=too_many.end() && assigned>112; ++it)
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

				if (assigned<=112) {
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

Fieres::Result Fieres::result(HICANNGlobal const& hicann) const
{
	size_t used=0;
	for (auto const& vline : mResult) {
		for (auto const& as : vline.second) {
			used+=as.drivers.size();
		}
	}
	MAROCCO_INFO("Synapse Drivers used: " << used <<  " on hicann: " << hicann << " " << mSide);

	return mResult;
}

Fieres::Rejected Fieres::rejected() const
{
	return mRejected;
}

} // namespace routing
} // namespace marocco