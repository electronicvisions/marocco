#pragma once

#include <vector>
#include <array>
#include <iosfwd>
#include <set>
#include <unordered_map>
#include <unordered_set>

#include <boost/icl/interval_map.hpp>
#include <boost/shared_ptr.hpp>

#include "marocco/config.h"
#include "marocco/annealer.h"
#include "marocco/RNG.h"
#include "marocco/routing/DriverAssignment.h"

namespace marocco {
namespace routing {
struct Reference
{
	/// references entry in DriverConfigurationState::mIntervals
	std::uint32_t pos;

	/// references entry in DriverInterval::intervals
	std::uint32_t psel;
};

inline
bool operator== (Reference const& lhs, Reference const& rhs)
{
	return lhs.pos == rhs.pos
		&& lhs.psel == rhs.psel;
}

} // namespace routing
} // namespace marocco

namespace std {
template<>
struct hash<marocco::routing::Reference>
{
	typedef marocco::routing::Reference type;
	size_t operator()(type const& e) const
	{
		size_t hash = e.pos;
		boost::hash_combine(hash, e.psel);
		return hash;
	}
};

} // namespace std

namespace marocco {
namespace routing {

/// Entries in the interval_map, have a count to count the number of assignments
/// for each interval. Furthermore, the list of references refers to
/// corresponding Synapse driver assignments.
struct Entry
{
	Entry() : overlap(0), refs() {}
	Entry(std::uint32_t pos, std::uint32_t psel) :
		overlap(1),
		refs()
	{
		refs.insert(Reference{pos, psel});
	}

    Entry& operator += (Entry const& rhs) {
        overlap += rhs.overlap;
        refs.insert(rhs.refs.begin(), rhs.refs.end());
        return *this;
    }

    Entry& operator -= (Entry const& rhs) {
        overlap -= rhs.overlap;
        for (auto const& val : rhs.refs) {
            refs.erase(val);
        }
        return *this;
    }

	operator int () const { return overlap; }

    int overlap;
	std::unordered_set<Reference> refs;
};

inline
bool operator== (Entry const& lhs, Entry const& rhs)
{
    return lhs.overlap == rhs.overlap
        && lhs.refs == rhs.refs;
}



struct DriverInterval
{
	typedef boost::icl::discrete_interval<int> Interval;

	DriverInterval(HMF::Coordinate::VLineOnHICANN const& vline,
				   size_t drivers, size_t syns=0);
	DriverInterval() = default;
	DriverInterval(DriverInterval const&) = default;

	/// corresponding line
	HMF::Coordinate::VLineOnHICANN const line;

	/// number of theoretically required driver to have no synapse loss
	size_t const driver;
	size_t const synapses;

	/// intervals associated with this route, can be two at max, one on top and
	/// one on bottom.
	typedef std::vector<std::pair<
			HMF::Coordinate::SynapseSwitchRowOnHICANN,
			std::pair<Interval, Entry>
		> > Vector;
	Vector intervals;

	friend std::ostream& operator<<(std::ostream& os, DriverInterval const& i);
};



struct DriverConfigurationState :
	public StateBase<DriverConfigurationState>
{
	typedef boost::icl::interval_map<int, Entry> IntervalMap;
	typedef DriverInterval::Interval Interval;
	typedef std::vector<DriverInterval> IntervalList;
	typedef RNG::result_type Seed;

	DriverConfigurationState(IntervalList const& list,
							 HMF::Coordinate::Side const& side,
							 size_t max_chain_length=5,
							 Seed s=0);

	static size_t length(Interval const& i);

	std::pair<HMF::Coordinate::SynapseSwitchRowOnHICANN, std::pair<Interval, Entry> >
	random_interval(HMF::Coordinate::VLineOnHICANN const& vline,
					size_t length, size_t pos, size_t psel);

	void next();
	static double temperature(size_t /*iter*/);
	double energy() const;
	static double thresh();

	/// Counts the amount of over-subscribed synapse drivers
	size_t countOverlap() const;

	/// Counts the total number of synapse driver which are scheduled for
	/// optimization. Can be less than initially requested, due to maximal chain
	/// length constraints.
	size_t countDrivers() const;

	/// Total number of requested synapse drivers, idependent of chain length
	/// limitations.
	size_t countRequested() const;

	/**
	 * post process driver assignment:
	 *  * resove overlaps
	 *  * assign yet unassigned drivers
	 *
	 *  @return returns list of rejected vlines
	 **/
	std::vector<HMF::Coordinate::VLineOnHICANN>
	postProcess();

	typedef std::unordered_map<
			HMF::Coordinate::VLineOnHICANN,
			std::vector<DriverAssignment>
		> Result;
	Result result(HMF::Coordinate::HICANNGlobal const& hicann) const;

	friend std::ostream& operator<<(std::ostream& os, DriverConfigurationState d);

private:
	int relative(HMF::Coordinate::SynapseSwitchRowOnHICANN const& s) const;
	IntervalMap& select(HMF::Coordinate::SynapseSwitchRowOnHICANN const& s);
	size_t driver(HMF::Coordinate::SynapseSwitchRowOnHICANN const& s) const;

	bool is_adjacent(HMF::Coordinate::VLineOnHICANN const& vline) const;

	void insertIntervals();

	size_t const MAX_CHAIN_LENGTH;

	IntervalList mIntervals;

	HMF::Coordinate::Side const mSide;
	double mSynapses;

	/// interval map for top and bottom synapse drivers. Left and right can be
	/// optimized indipendently.
	std::array<IntervalMap, 2> mIntMaps;

	boost::shared_ptr<RNG> mRNG;
};

} // namespace routing
} // namespace marocco
