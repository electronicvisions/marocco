#pragma once

#include <array>
#include <iosfwd>
#include <functional>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/nvp.hpp>

#ifndef PYPLUSPLUS
#include "marocco/graph.h"
#endif // !PYPLUSPLUS

namespace euter {
class Population;
}

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

#ifndef PYPLUSPLUS
	typedef graph_t::vertex_descriptor value_type;
#endif // !PYPLUSPLUS
	typedef size_t population_type;
	typedef int32_t mask_element_type;
	typedef std::vector<mask_element_type> mask_type;

#ifndef PYPLUSPLUS
	PopulationSlice(value_type val, euter::Population const& pop);
	PopulationSlice(value_type val, size_t offset, size_t size);
#endif // !PYPLUSPLUS

	size_t offset() const;
	size_t size() const;
	bool empty() const;

#ifndef PYPLUSPLUS
	/** Population vertex in pynn graph.
	 */
	value_type const& population() const;
#endif // !PYPLUSPLUS

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

	size_t hash() const;

	/**
	 *  @brief Slice populations into pieces (PopulationSlices) according to selection mask (and
	 *  ignore the rest)
	 *  @return Vector of PopulationSlices
	 */
	 static std::vector<PopulationSlice> slice_by_mask(mask_type mask, PopulationSlice slice);

	 /**
	 *  @brief Inverts mask
         *  @param max_size Size of Population the mask describes
         *  @return Inverted mask of size max_size
	 *  @note Mask must be sorted.
         */
	 static mask_type invert_mask(mask_type mask, mask_element_type max_size);

private:
	// Default constructor needed for serialization
	PopulationSlice();

	friend class boost::serialization::access;

#ifndef PYPLUSPLUS
	template <typename Archiver>
	void serialize(Archiver& ar, unsigned int const /*version*/) {
		using boost::serialization::make_nvp;
		ar & make_nvp("value", mValue)
		   & make_nvp("offset", mOffset)
		   & make_nvp("size", mSize);
	}

	value_type mValue;
#endif // !PYPLUSPLUS
	size_t mOffset;
	size_t mSize;
};

std::ostream& operator<<(std::ostream& os, PopulationSlice const& b);
bool operator==(PopulationSlice const& lhs, PopulationSlice const& rhs);
bool operator!=(PopulationSlice const& lhs, PopulationSlice const& rhs);
size_t hash_value(PopulationSlice const& ps);


} // namespace assignment
} // namespace marocco

namespace std {
template <>
struct hash<marocco::assignment::PopulationSlice> {
	size_t operator()(marocco::assignment::PopulationSlice const& b) const { return hash_value(b); }
};
} // namespace std
