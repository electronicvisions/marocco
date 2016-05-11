#pragma once

#include <array>
#include <iosfwd>
#include <functional>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/nvp.hpp>

#include "marocco/graph.h"

class Population;

namespace marocco {
namespace assignment {

/** Part of a population to be assigned to hardware neurons (also used
 *  for spike input).  As populations may be too big to be assigned to
 *  adjacent neurons they have to be split into chunks. This class
 *  represents a slice of `size()` bio neurons starting at `offset()`
 *  in the respective population.
 */
struct PopulationSlice {
public:
	typedef graph_t::vertex_descriptor value_type;

	PopulationSlice(value_type val, Population const& pop);
	PopulationSlice(value_type val, size_t offset, size_t size);

	size_t offset() const;
	size_t size() const;
	bool empty() const;

	/** Population vertex in pynn graph.
	 */
	value_type const& population() const;

	/** Split off a slice of bio neurons at the front.
	 *  @return \c PopulationSlice of the \c n first bio neurons (at most).
	 *  @note The current slice will contain the remaining neurons (if any).
	 */
	PopulationSlice slice_front(size_t n);

	/** Split off a slice of bio neurons at the back.
	 *  @return \c PopulationSlice of the \c n last bio neurons (at most).
	 *  @note The current slice will contain the remaining neurons (if any).
	 */
	PopulationSlice slice_back(size_t n);

	/** Return two equal-sized slices.
	 */
	std::array<PopulationSlice, 2> split() const;

private:
	// Default constructor needed for serialization
	PopulationSlice();

	friend class boost::serialization::access;

	template <typename Archiver>
	void serialize(Archiver& ar, unsigned int const /*version*/) {
		using boost::serialization::make_nvp;
		ar & make_nvp("value", mValue)
		   & make_nvp("offset", mOffset)
		   & make_nvp("size", mSize);
	}

	value_type mValue;
	size_t mOffset;
	size_t mSize;
};

std::ostream& operator<<(std::ostream& os, PopulationSlice const& b);
bool operator==(PopulationSlice const& lhs, PopulationSlice const& rhs);
bool operator!=(PopulationSlice const& lhs, PopulationSlice const& rhs);

} // namespace assignment
} // namespace marocco

namespace std {
template <>
struct hash<marocco::assignment::PopulationSlice> {
	size_t operator()(marocco::assignment::PopulationSlice const& b) const {

		size_t hash = 0;
		boost::hash_combine(hash, b.population());
		boost::hash_combine(hash, b.size());
		boost::hash_combine(hash, b.offset());
		return hash;
	}
};
} // namespace std
