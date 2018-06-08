#include "marocco/placement/OnNeuronBlock.h"

#include <algorithm>

namespace marocco {
namespace placement {

OnNeuronBlock::OnNeuronBlock()
	: mDefect(std::make_shared<NeuronPlacement>(assignment::PopulationSlice({}, 0, 0), 2)),
	  mAssignment(),
	  mAvailable(neuron_coordinate::enum_type::size) {}

void OnNeuronBlock::add_defect(neuron_coordinate const& nrn) {
	auto& ptr = mAssignment[nrn.x()][nrn.y()];
	if (ptr) {
		throw std::runtime_error("NeuronOnNeuronBlock already taken.");
	}
	ptr = mDefect;
	--mAvailable;
}

auto OnNeuronBlock::begin() const -> iterator {
	return {mAssignment.cbegin()->begin(), mAssignment.cend()->begin(), mDefect};
}

auto OnNeuronBlock::end() const -> iterator {
	auto const& end = mAssignment.cend()->begin();
	return {end, end, mDefect};
}

auto OnNeuronBlock::neurons(iterator const& it) const -> iterable<neuron_iterator> {
	auto const& beg = mAssignment.cbegin()->begin();
	auto const& end = mAssignment.cend()->begin();
	return {neuron_iterator{it.base(), beg, end}, neuron_iterator{end, beg, end}};
}

auto OnNeuronBlock::add(NeuronPlacement const& value) -> iterator {
	constexpr size_t height = neuron_coordinate::y_type::size;
	size_t const size = value.size();

	assert(size % 2 == 0);

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
			std::fill(begin, end, std::make_shared<NeuronPlacement>(value));
			mAvailable -= size;
			return {begin, mAssignment.cend()->begin(), mDefect};
		} else {
			begin = end;
		}
	}

	return end();
}

auto OnNeuronBlock::operator[](neuron_coordinate const& nrn) const -> value_type {
	auto ptr = mAssignment[nrn.x()][nrn.y()];
	if (ptr != mDefect) {
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

	return {first, mAssignment.cend()->begin(), mDefect};
}

bool OnNeuronBlock::empty() const {
	return mAvailable == neuron_coordinate::enum_type::size;
}

size_t OnNeuronBlock::available() const { return mAvailable; }

namespace detail {
namespace on_neuron_block {

iterator::iterator(OnNeuronBlock::base_iterator const& it,
                   OnNeuronBlock::base_iterator const& end,
                   OnNeuronBlock::value_type const& defect)
	: iterator_adaptor_(it), mEnd(end), mDefect(defect) {
	auto const& val = *this->base();
	if (!val || val == mDefect) {
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
		if (val               // not empty
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
	if (!val || val != last) {
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
