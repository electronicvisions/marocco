#pragma once

#include <vector>
#include <array>
#include <iosfwd>
#include <set>
#include <unordered_set>

#include <boost/icl/interval_map.hpp>

#include "hal/Coordinate/L1.h"
#include "hal/Coordinate/Synapse.h"

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

} // namespace routing
} // namespace marocco
