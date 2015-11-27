#include "marocco/assignment/NeuronBlockSlice.h"

namespace marocco {
namespace assignment {

NeuronBlockSlice::NeuronBlockSlice(
	coordinate_type coordinate_, offset_type offset_, size_type size_, size_type neuron_size_)
	: mCoordinate(coordinate_), mOffset(offset_), mSize(size_), mHwNeuronSize(neuron_size_)
{
	assert(size() >= neuron_size());
	assert((size() % neuron_size()) == 0);
	assert(size() <= offset_type::enum_type::size);
}

NeuronBlockSlice::NeuronBlockSlice()
{
}

auto NeuronBlockSlice::offset() const -> offset_type
{
	return mOffset;
}

auto NeuronBlockSlice::size() const -> size_type
{
	return mSize;
}

auto NeuronBlockSlice::neuron_size() const -> size_type
{
	return mHwNeuronSize;
}

auto NeuronBlockSlice::coordinate() const -> coordinate_type const&
{
	return mCoordinate;
}

} // namespace assignment
} // namespace marocco
