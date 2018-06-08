#include "marocco/routing/SynapseDriverSA.h"
#include "marocco/routing/util.h"
#include "marocco/Logger.h"

#include <unordered_map>
#include <cmath>
#include <memory>
#include <set>
#include <ostream>

using namespace HMF::Coordinate;

namespace marocco {
namespace routing {

namespace {

// TODO: could be replaced by marocco::routing::Reference & assignment counter
struct DriverProxy
{
	typedef DriverConfigurationState::IntervalList IntervalList;
	typedef DriverInterval::Vector Vector;

	DriverProxy(DriverProxy const&) = default;
	DriverProxy(IntervalList::const_iterator it,
				Vector::const_iterator sel) :
		it(it), sel(sel), assigned(1)
	{}

	IntervalList::const_iterator it;
	Vector::const_iterator sel;
	size_t assigned = 1;

	size_t length() const
	{
		return DriverConfigurationState::length(sel->second.first);
	}

	size_t synapses() const
	{
		return it->synapses;
	}

	// only for std::map ordering
	bool operator< (DriverProxy const& rhs) const
	{
		return length() < rhs.length();
	}

	bool operator== (DriverProxy const& rhs) const
	{
		return it == rhs.it
			&& sel == rhs.sel
			&& assigned == rhs.assigned;
	}
};

struct CmpSharedPtrByValueInverse
{
	template<typename T>
	bool operator() (std::shared_ptr<T> const& a,
					 std::shared_ptr<T> const& b)
	{
		return *b < *a;
	}
};

} // namespace

DriverInterval::DriverInterval(VLineOnHICANN const& vline, size_t drivers, size_t syns) :
	line(vline),
	driver(drivers),
	synapses(syns),
	intervals()
{
	if (!driver) {
		throw std::runtime_error("empty driver interval");
	}
}

std::ostream& operator<<(std::ostream& os, DriverInterval const& i)
{
	os << "DriverInterval(" <<  i.line << " driver: "
		<< i.driver << " inserts: " << i.intervals.size() << " synapses: " << i.synapses;
	for(auto const& val : i.intervals) {
		os << val.first << " ";
	}
	return os << ")";
}

DriverConfigurationState::DriverConfigurationState(IntervalList const& list,
												   Side const& side,
												   size_t max_chain_length,
												   Seed s) :
	MAX_CHAIN_LENGTH(max_chain_length),
	mIntervals(list),
	mSide(side),
	mSynapses(0),
	mRNG(new RNG(s))
{
	// initialize intervals
	for (size_t pos=0; pos<mIntervals.size(); ++pos)
	{
		auto& val = mIntervals[pos];
		mSynapses += val.synapses;
		int driver = val.driver;
		int cnt = 0;
		while(driver>0 && cnt<2) {
			size_t insert = std::min<size_t>(MAX_CHAIN_LENGTH, size_t(driver));
			val.intervals.push_back(random_interval(val.line, insert, pos, cnt));
			driver-=insert;
			cnt++;
		}
	}

	// insert into interval tree
	insertIntervals();
}

size_t DriverConfigurationState::length(Interval const& i)
{
	return i.upper() - i.lower();
}

std::pair<SynapseSwitchRowOnHICANN, std::pair<DriverConfigurationState::Interval, Entry> >
DriverConfigurationState::random_interval(
	VLineOnHICANN const& vline,
	size_t const length,
	size_t const pos,
	size_t const psel)
{
	// FIXME: getReachableSwitchRows is known to be broken...
	SynapseSwitchRowOnHICANN const& row =
		vline.getReachableSwitchRows(mSide)[(*mRNG)()%14];

	int const rel    = relative(row);
	int const offset = (*mRNG)()%length;

	int lower = std::max(0, rel-offset);
	int upper = lower+length;
	if (upper>56) {
		upper = 56;
		lower = upper-length;
	}

	if (upper-lower!=int(length)) {
		throw std::runtime_error("incorrect length");
	}
	if (upper<=rel || lower>rel) {
		throw std::runtime_error("range error");
	}

	return std::make_pair(row, std::make_pair(Interval::right_open(lower, upper), Entry(pos, psel)));
}

void DriverConfigurationState::next()
{
	size_t const overlap = countOverlap();
	if (!overlap) {
		throw std::runtime_error("no overlap, nothing to do");
	}

	// randomly start with up or down block
	size_t const pos = (*mRNG)() % mIntMaps.size();

	// try both blocks if first has NO overlap
	for (size_t kk=0; kk<mIntMaps.size(); ++kk)
	{
		IntervalMap& map = mIntMaps[(pos+kk)%mIntMaps.size()];

		// find interval with maximum overlap
		auto it = map.begin();
		size_t max=0;
		decltype(it) max_it=map.end();


		//std::vector<decltype(it)> its;
		//for(; it!=map.end(); ++it)
		//{
			//size_t const len = it->first.upper() - it->first.lower();
			//size_t const vol = len * (it->second.overlap-1);

			//if(vol>=1) {
				//its.push_back(it);
			//}
		//}

		//if (!its.empty()) {
			//max_it = its[(*mRNG)() % its.size()];
		//}



		for(; it!=map.end(); ++it)
		{
			size_t const len = it->first.upper() - it->first.lower();
			size_t const vol = len * (it->second.overlap-1);

			if(vol>max) {
				max=vol;
				max_it=it;
			}
		}

		if (max_it!=map.end()) {
			// collect all assignments
			std::vector<Reference> lookup;
			for (auto const& ref : max_it->second.refs)
			{
				lookup.push_back(ref);
			}

			// pick random assignment for optimization
			size_t const _p = (*mRNG)() % lookup.size();
			Reference const ref = lookup[_p];

			DriverInterval& ival = mIntervals.at(ref.pos);
			DriverInterval::Vector::value_type& entry = ival.intervals.at(ref.psel);
			size_t const len = length(entry.second.first);


			// remove old assignment
			map -= entry.second;

			// generate new assignment and insert
			entry = random_interval(ival.line, len, ref.pos, ref.psel);

			mIntMaps[entry.first.line()<112 ? 0 : 1] += entry.second;

			return;
		}
	}

	throw std::runtime_error("could not propagate state");
}

double DriverConfigurationState::temperature(size_t iter)
{
	return 20.*exp(-double(iter)/2500.);
}

double DriverConfigurationState::thresh()
{
	// we can abort early if there is no SynapseDriver overlap
	return 0;
}

double DriverConfigurationState::energy() const
{
	// energy of current state represented by two terms:
	//  * penalty for overassigned synapse driver
	//  * number of unused driver
	size_t overlap = countOverlap();
	if (!overlap) {
		return 0;
	}

	// higher energy for overlapping primary synapse driver
	std::unordered_map<HMF::Coordinate::SynapseSwitchRowOnHICANN, size_t> primary_overlap_;
	for (auto const& ival : mIntervals) {
		for (auto const& i : ival.intervals) {
			primary_overlap_[i.first] += 1;
		}
	}
	double E_primary=0;
	for (auto const& entry : primary_overlap_) {
		E_primary+=entry.second;
	}
	// subtract base line for single assignments
	E_primary-=primary_overlap_.size();


	// count unused synapse driver
	//double E_unused=0;
	//for (auto const& map : mIntMaps) {
		//int last_upper = 0;
		//for (auto const& iv : map) {
			//// iterator counts only assigned intervals, hence skiped periods are
			//// unassigned drivers.
			//if (iv.first.lower()!=last_upper) {
				//E_unused+=(iv.first.lower()-last_upper);
			//}
			//last_upper = iv.first.upper();
		//}
		//E_unused+=(56-last_upper);
	//}

	//return E_primary*20 + E_unused + overlap;
	return E_primary*20 + overlap;
}

int DriverConfigurationState::relative(SynapseSwitchRowOnHICANN const& s) const
{
	return (driver(s))%56 /* num_syndrv_per_block */;
}

DriverConfigurationState::IntervalMap&
DriverConfigurationState::select(SynapseSwitchRowOnHICANN const& s)
{
	return mIntMaps[s.line()>111 ? 1 : 0];
}

size_t DriverConfigurationState::driver(SynapseSwitchRowOnHICANN const& s) const
{
	return s.line()/2;
}

size_t DriverConfigurationState::countOverlap() const
{
	size_t overlap = 0;
	for (auto const& map : mIntMaps) {
		for (auto const& iv : map) {
			// one assignment is not yet an overlap
			if (iv.second>1) {
				overlap += length(iv.first)*iv.second;
			}
		}
	}
	return overlap;
}

size_t DriverConfigurationState::countDrivers() const
{
	size_t driver = 0;
	for (auto const& map : mIntMaps) {
		for (auto const& iv : map) {
			driver += length(iv.first)*iv.second;
		}
	}
	return driver;
}

size_t DriverConfigurationState::countRequested() const
{
	size_t requested = 0;
	for (auto const& req : mIntervals) {
		requested += req.driver;
	}
	return requested;
}


std::vector<VLineOnHICANN>
DriverConfigurationState::postProcess()
{
	std::vector<VLineOnHICANN> reject;

	// minor consistency check (DEBUG only)
	for (auto const& topbot: mIntMaps) {
		for (auto const& iv : topbot) {
			int const count = iv.second;
			if (count<0) {
				throw std::runtime_error("negative number of synapse drivers assigned");
			}
		}
	}

	// First, check whether we have any overlap at all. Otherwise, we can simply
	// skip the post Processing. At least as long as there is no assignment os
	// remaining, unused synapse drivers. That post processing step could still
	// be applied.
	size_t overlap = countOverlap();
	if (!overlap) {
		return reject;
	}

	// We can have only one route assigned to one synapse row. For all
	// redundant keep the one with bigger interval and get rid of the other.
	std::unordered_map<SynapseSwitchRowOnHICANN, DriverProxy> accepted;
	std::map<DriverInterval::Vector::const_iterator, bool> _mapped;
	for (auto it=mIntervals.cbegin(); it!=mIntervals.cend(); ++it)
	{
		for (auto iit=it->intervals.cbegin(); iit!=it->intervals.cend(); ++iit)
		{
			SynapseSwitchRowOnHICANN const& row = iit->first;
			auto sit = accepted.find(row);
			if (sit==accepted.end()) {
				accepted.insert(std::make_pair(row, DriverProxy(it, iit)));
				_mapped[iit] = true;
			} else {
				// here, other objectives might be implemented than synapse loss
				// minimization.
				if(length(iit->second.first) > length(sit->second.sel->second.first))
				{
					_mapped[sit->second.sel] = false;
					_mapped[iit] = true;
					sit->second = DriverProxy(it, iit);
				} else {
					_mapped[iit] = false;
				}
			}
		}
	}

	for (auto it=mIntervals.cbegin(); it!=mIntervals.cend(); ++it)
	{
		size_t m=0;
		for (auto iit=it->intervals.cbegin(); iit!=it->intervals.cend(); ++iit)
		{
			m += _mapped.at(iit) ? 1 : 0;
		}

		// this route has no insertion point
		if (!m) {
			VLineOnHICANN const vline = it->line;
			reject.push_back(vline);
		}
	}


	// not we have a complete list of accepted primary synapse driver
	// assignments. We now need to figure out WHO gets WHICH other adjacent
	// drivers.
	std::array<std::array<std::shared_ptr<DriverProxy>, 56>, 2> assignment;
	for (auto const& val : accepted)
	{
		auto sp = std::make_shared<DriverProxy>(val.second);
		//if (val.second.length()>1) {
			//to_grow.insert(sp);
		//}
		if (val.second.length()>MAX_CHAIN_LENGTH) {
			std::runtime_error("this should not happen");
		}
		size_t const pos = driver(val.second.sel->first);
		assignment.at(pos>=56).at(pos%56) = sp;
	}

	for (auto& topbot : assignment)
	{
		for (size_t row=0; row<topbot.size(); ++row)
		{
			auto& ptr = topbot[row];

			if (!ptr) {
				continue;
			} else {
				DriverProxy& proxy = *ptr;

				size_t const length = proxy.length();
				int top = std::max<int>(0, row-length+1);
				int bot = std::min<int>(topbot.size()-1, row+length);

				// look above
				for (int ii=row-1; ii>=top && proxy.assigned < length; --ii)
				{
					auto& _ptr = topbot[ii];
					if (!_ptr) {
						_ptr = ptr;
						proxy.assigned++;

					} else {
						break;
					}
				}

				// look below
				for (int ii=row+1; ii<=bot && proxy.assigned < length; ++ii)
				{
					auto& _ptr = topbot[ii];
					if (!_ptr) {
						_ptr = ptr;
						proxy.assigned++;

					} else {
						break;
					}
				}
			}
		}
	}


	// we can now try to assign remaining synapse drivers
	// TODO: this feature is missing. implementer should think about reliability
	// if synapse driver chains get longer than 5 drivers. This might not only
	// affect drivers far apart, but may also make drivers close to the primary
	// more unreliable due to capacitive load.

	// update interval map to reflect our assignment
	std::unordered_map<HMF::Coordinate::VLineOnHICANN, DriverInterval> tmp;
	for (auto const& block : assignment)
	{
		auto pos = [&block](std::array<std::shared_ptr<DriverProxy>, 56>::const_iterator it) {
			size_t const p = it - block.begin();
			if (p > block.size()) {
				throw std::runtime_error("out of range");
			}
			return p;
		};

		auto it = block.cbegin();
		while (it!=block.cend())
		{
			auto const& driver = *it;
			if (driver)
			{
				size_t const lower = pos(it);
				while (++it!=block.cend() && *it == driver) { /*nothing*/ }
				size_t const upper = pos(it);

				VLineOnHICANN const& vline = driver->it->line;
				auto iit = tmp.find(vline);
				if (iit == tmp.end()) {
					bool suc = false;
					std::tie(iit, suc) = tmp.insert(std::make_pair(vline, DriverInterval(vline, driver->it->driver)));
					if (!suc) {
						throw std::runtime_error("SynapseDriverSA: WTF!");
					}
				}
				DriverInterval& ival = iit->second;

				std::unordered_set<Reference> const& refs = driver->sel->second.second.refs;
				if (refs.size() != 1) {
					throw std::runtime_error("WTF");
				}

				// FIXME: back references have to be fixed up later on
				ival.intervals.push_back(std::make_pair(driver->sel->first, std::make_pair(
					Interval::right_open(lower, upper), Entry(-1, -1))));
			} else {
				++it;
			}
		}
	}

	// construct new interval list and update local representation
	IntervalList updated_list;
	updated_list.reserve(tmp.size());
	for (auto const& entry : tmp) {
		updated_list.push_back(entry.second);
	}
	mIntervals = std::move(updated_list);


	// clear local interval maps before inserting new intervals
	for (auto& val : mIntMaps) {
		val.clear();
	}
	insertIntervals();

	return reject;
}

DriverConfigurationState::Result
DriverConfigurationState::result(HICANNGlobal const& hicann) const
{
	Result result;
	size_t used = 0;

	// DEBUG: make sure we have a proper result
	size_t const overlap = countOverlap();
	if (overlap) {
		throw std::runtime_error("no proper result. did you forget to call postProcess()?");
	}

	for (DriverInterval const& ival : mIntervals)
	{
		bool const is_adj = is_adjacent(ival.line);

		std::vector<DriverAssignment>& assign = result[ival.line];
		for (auto const& a : ival.intervals)
		{
			DriverAssignment as;
			SynapseSwitchRowOnHICANN const& row = a.first;

			SynapseDriverOnHICANN primary;
			if (!is_adj) {
				primary = row.toSynapseDriverOnHICANN();
			} else {
				if ((row.line()<112) xor row.toSideHorizontal()) {
					// top left or bottom right
					primary = SynapseDriverOnHICANN(Y(row.line().value()+1), mSide);
				} else {
					primary = SynapseDriverOnHICANN(Y(row.line().value()-1), mSide);
				}
			}

			as.primary = primary;

			// base of set (top/bottom block) plus offset of 1 depending on
			// sector.
			size_t offset = row.line()>=112 ? 112 : 0;
			offset += ((row.line()<112) xor row.toSideHorizontal());
			for (int ii=a.second.first.lower(); ii<a.second.first.upper(); ++ii)
			{
				as.drivers.insert(as.drivers.end(), DriverAssignment::Driver(Y(offset+2*ii), mSide));
			}

			used+=as.drivers.size();

			assign.push_back(as);
		}
	}


	MAROCCO_INFO("Synapse Drivers used: " << used <<  " on hicann: " << hicann << " " << mSide);

	return result;
}

bool DriverConfigurationState::is_adjacent(VLineOnHICANN const& vline) const
{
	return vline.toSideHorizontal() != mSide;
}

void DriverConfigurationState::insertIntervals()
{
	for (auto& val : mIntervals) {
		for (auto const& i : val.intervals) {
			select(i.first) += i.second;
		}
	}
}

std::ostream& operator<<(std::ostream& os, DriverConfigurationState d)
{
	for (auto const& topbot : d.mIntMaps) {
		for (auto const& ival : topbot) {
			for (int ii=ival.first.lower(); ii<ival.first.upper(); ++ii) {
				os.width(3);
				os << ii;
				os << ": " << ival.second << "\n";
			}
		}
	}
	for (auto const& iv : d.mIntervals) {
		os << iv << "\n";
	}
	return os;
}

} // namespace routing
} // namespace marocco
