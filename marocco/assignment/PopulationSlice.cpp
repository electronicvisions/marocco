#include "marocco/assignment/PopulationSlice.h"
#include "euter/population.h"

#include <ostream>
#include <stdexcept>

namespace marocco {
namespace assignment {

PopulationSlice::PopulationSlice() {}

PopulationSlice::PopulationSlice(value_type val, euter::Population const& pop)
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

std::vector<PopulationSlice> PopulationSlice::slice_by_mask(
	mask_type mask, PopulationSlice slice)
{
	std::vector<PopulationSlice> result;
	size_t offset = slice.offset();
	size_t slice_size = 1; // since loop starts with second element of mask
	if(mask.empty()) {
		return result;
	}
	assert(mask.back()-offset<=slice.size());
	assert(mask.front()>=static_cast<int32_t>(offset));
	// remove all neurons up to first neuron specified in mask
	// Note: mask describes neurons in Population, PopulationsSlice might start with offset -> substract offset
	slice.slice_front(mask[0]-offset);
	// slice PopulationSlice into as few contiguous slices as possible which are specified in mask
	for (size_t i = 1; i < mask.size(); i++) {
		if(mask[i] != mask[i-1] + 1) { // true if current and previous mask element not contiguous
			// store current PopulationSlice in result vector and remove its neurons from slice
			result.push_back(slice.slice_front(slice_size));
			// remove all neurons from slice up to next neuron which is specified in mask
			slice.slice_front(mask[i]-mask[i-1]-1);
			slice_size = 0; // start new PopulationSlice
		}
		slice_size++; // current and previous neuron still contiguous -> increase current slice
	}
	result.push_back(slice.slice_front(slice_size)); // store last evaluated neurons
	return result;
}

PopulationSlice::mask_type PopulationSlice::invert_mask(PopulationSlice::mask_type mask,PopulationSlice::mask_element_type max_size)
{
	PopulationSlice::mask_type full_mask(max_size);
	// fill full mask with values from 0 to max_size-1
	std::iota(full_mask.begin(), full_mask.end(), 0);
	PopulationSlice::mask_type inverted_mask;
	// delete all elements from full_mask which are specified in mask
	std::set_difference(full_mask.begin(), full_mask.end(), mask.begin(), mask.end(), std::back_inserter(inverted_mask));
	return inverted_mask;
}

} // namespace assignment
} // namespace marocco
