#include "marocco/assignment/Hardware.h"

namespace marocco {
namespace assignment {

Hardware::Hardware(value_type assign,
				   offset_type offset,
				   size_type size,
				   size_type hw_neuron_size) :
	mAssign(assign),
	mOffset(offset),
	mSize(size),
	mHWNeuronSize(hw_neuron_size)
{}

Hardware::Hardware() {}

Hardware::size_type Hardware::size() const
{
	return mSize;
}

Hardware::offset_type Hardware::offset() const
{
	return mOffset;
}

Hardware::value_type const& Hardware::get() const
{
	return mAssign;
}

/// returns number hardware neuron modules used for one neuron
Hardware::size_type Hardware::hw_neuron_size() const
{
	return mHWNeuronSize;
}

} // namespace assignment
} // namespace marocco
