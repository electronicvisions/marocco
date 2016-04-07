#include "marocco/placement/OnNeuronBlock.h"

#include <algorithm>
#include <limits>
#include <stdexcept>

#include "marocco/util.h"

namespace marocco {
namespace placement {

OnNeuronBlock::OnNeuronBlock()
	: mAssignment(),
	  mSize(0),
	  mAvailable(neuron_coordinate::enum_type::size),
	  mCeiling(std::numeric_limits<size_t>::max())
{
}

auto OnNeuronBlock::defect_marker() -> value_type
{
	static value_type defect =
	    std::make_shared<NeuronPlacementRequest>(assignment::PopulationSlice({}, 0, 0), 2);
	return defect;
}

void OnNeuronBlock::add_defect(neuron_coordinate const& nrn) {
	if (!empty()) {
		throw std::runtime_error("OnNeuronBlock: add_defect() called after add().");
	}

	auto& ptr = mAssignment[nrn.x()][nrn.y()];
	if (assigned_p(ptr)) {
		throw ResourceInUseError("NeuronOnNeuronBlock already taken.");
	}
	ptr = defect_marker();
	--mAvailable;
}

auto OnNeuronBlock::begin() const -> iterator {
	return {mAssignment.cbegin()->begin(), mAssignment.cend()->begin(), defect_marker()};
}

auto OnNeuronBlock::end() const -> iterator {
	auto const& end = mAssignment.cend()->begin();
	return {end, end, defect_marker()};
}

auto OnNeuronBlock::neurons(iterator const& it) const -> iterable<neuron_iterator> {
	auto const& beg = mAssignment.cbegin()->begin();
	auto const& end = mAssignment.cend()->begin();
	return {neuron_iterator{it.base(), beg, end}, neuron_iterator{end, beg, end}};
}

auto OnNeuronBlock::add(NeuronPlacementRequest const& value) -> iterator {
	size_t const size = value.size();

	if (size > available()) {
		return end();
	}

	// This should be enforced in NeuronPlacementRequest, so an assertion is enough here.
	assert(size % 2 == 0);

	constexpr size_t height = neuron_coordinate::y_type::size;

	auto const BEGIN = mAssignment.begin()->begin();
	auto const END = mAssignment.end()->begin();
	auto begin = BEGIN;

	while (begin < END) {
		begin = std::find_if(begin, END, unassigned_p);

		// Only start assignments at top neuron row.
		size_t y = std::distance(BEGIN, begin) % height;
		if (y > 0) {
			std::advance(begin, height - y);
			continue;
		}

		auto end = std::find_if(begin, END, assigned_p);
		size_t available = std::distance(begin, end);
		if (size <= available) {
			end = begin;
			std::advance(end, size);
			std::fill(begin, end, std::make_shared<NeuronPlacementRequest>(value));
			mSize += size;
			return {begin, mAssignment.cend()->begin(), defect_marker()};
		} else {
			begin = end;
		}
	}

	return end();
}

auto OnNeuronBlock::operator[](neuron_coordinate const& nrn) const -> value_type {
	auto ptr = mAssignment[nrn.x()][nrn.y()];
	if (ptr != defect_marker()) {
		return ptr;
	}
	return {};
}

auto OnNeuronBlock::get(neuron_coordinate const& nrn) const -> iterator {
	auto const BEGIN = mAssignment.begin()->begin();
	auto it = mAssignment[nrn.x()].begin();
	std::advance(it, nrn.y());

	auto first = it;
	for (--it; it >= BEGIN; --it) {
		if (*it == *first) {
			first = it;
		}
	}

	return {first, mAssignment.cend()->begin(), defect_marker()};
}

bool OnNeuronBlock::empty() const
{
	return !mSize;
}

size_t OnNeuronBlock::available() const
{
	return std::min(mAvailable, mCeiling) - mSize;
}

size_t OnNeuronBlock::restrict(size_t max_denmems)
{
	if (!empty()) {
		throw std::runtime_error("OnNeuronBlock: restrict() called after add().");
	}

	mCeiling = std::min(mCeiling, max_denmems);
	return mCeiling;
}

namespace detail {
namespace on_neuron_block {

iterator::iterator(OnNeuronBlock::base_iterator const& it,
                   OnNeuronBlock::base_iterator const& end,
                   OnNeuronBlock::value_type const& defect)
	: iterator_adaptor_(it), mEnd(end), mDefect(defect) {
	if (this->base() == mEnd) {
		return;
	}
	auto const& val = *this->base();
	if (val == nullptr || val == mDefect) {
		increment();
	}
}

void iterator::increment() {
	if (this->base() == mEnd) {
		return;
	}

	auto const& last = *this->base();
	++(this->base_reference());

	while (this->base() != mEnd) {
		auto const& val = *this->base();
		if (val != nullptr    // not empty
		    && val != last    // new population
		    && val != mDefect // no defect
		    ) {

			break;
		}

		++(this->base_reference());
	}
}

neuron_iterator::neuron_iterator(OnNeuronBlock::base_iterator const& it,
                                 OnNeuronBlock::base_iterator const& beg,
                                 OnNeuronBlock::base_iterator const& end)
	: iterator_adaptor_(it), mBeg(beg), mEnd(end) {}

void neuron_iterator::increment() {
	if (this->base() == mEnd) {
		return;
	}

	auto const& last = *this->base();
	++(this->base_reference());

	auto const& val = *this->base();
	if (val == nullptr || val != last) {
		this->base_reference() = mEnd;
	}
}

OnNeuronBlock::neuron_coordinate neuron_iterator::dereference() const {
	size_t diff = std::distance(mBeg, this->base());
	constexpr size_t height = OnNeuronBlock::neuron_coordinate::y_type::size;
	return OnNeuronBlock::neuron_coordinate{HMF::Coordinate::X{diff / height},
	                                        HMF::Coordinate::Y{diff % height}};
}

} // namespace on_neuron_block
} // namespace detail

} // namespace placement
} // namespace marocco
