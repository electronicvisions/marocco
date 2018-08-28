#include "marocco/assignment/PopulationSlice.h"

#include <ostream>
#include <stdexcept>

namespace marocco {
namespace assignment {

PopulationSlice::PopulationSlice() {}

PopulationSlice::PopulationSlice(value_type val, Population const& pop)
	: PopulationSlice(val, 0, pop.size()) {}

PopulationSlice::PopulationSlice(value_type val, size_t offset, size_t size)
    : mValue(val), mOffset(offset), mSize(size) {}

size_t PopulationSlice::offset() const {
	return mOffset;
}

size_t PopulationSlice::size() const {
	return mSize;
}

bool PopulationSlice::empty() const {
	return !mSize;
}

auto PopulationSlice::population() const -> value_type const& {
	return mValue;
}

PopulationSlice PopulationSlice::slice_front(size_t n) {
	n = std::min(n, mSize);
	PopulationSlice first(mValue, mOffset, n);
	mOffset += n;
	mSize -= n;
	return first;
}

PopulationSlice PopulationSlice::slice_back(size_t n) {
	n = std::min(n, mSize);
	mSize -= n;
	return {mValue, mOffset + mSize, n};
}

std::array<PopulationSlice, 2> PopulationSlice::split() const {
	if (mSize < 2) {
		throw std::out_of_range(
			"not enough neurons to split population");
	}

	size_t const second = mSize / 2;
	size_t const first = mSize - second;
	return {{{mValue, mOffset, first}, {mValue, mOffset + first, second}}};
}

std::ostream& operator<<(std::ostream& os, PopulationSlice const& b) {
	os << "asssignment::PopulationSlice(" << b.population() << ", " << b.offset() << ", "
	   << b.size() << ")";
	return os;
}

bool operator==(PopulationSlice const& lhs, PopulationSlice const& rhs) {
	return (lhs.population() == rhs.population() && lhs.offset() == rhs.offset() &&
	        lhs.size() == rhs.size());
}

bool operator!=(PopulationSlice const& lhs, PopulationSlice const& rhs) {
	return !(lhs == rhs);
}

size_t PopulationSlice::hash() const
{
	size_t hash = 0;
	boost::hash_combine(hash, mValue);
	boost::hash_combine(hash, mOffset);
	boost::hash_combine(hash, mSize);
	return hash;
}

size_t hash_value(PopulationSlice const& ps)
{
	return ps.hash();
}

} // namespace assignment
} // namespace marocco
