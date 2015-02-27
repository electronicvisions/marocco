#pragma once

#include "sthal/macros.h"
#include "hal/Coordinate/HMFGeometry.h"

namespace marocco {
namespace parameter {

template <typename T>
class NeuronOnHICANNPropertyArray
{
public:
	typedef HMF::Coordinate::NeuronOnHICANN neuron_coordinate;
	typedef T value_type;
	typedef std::array<std::array<value_type, neuron_coordinate::x_type::size>,
	                   neuron_coordinate::y_type::size> array_type;

	STHAL_ARRAY_OPERATOR(value_type, neuron_coordinate, return mArray[ii.y()][ii.x()];)
protected:
	array_type mArray;
};

} // namespace parameter
} // namespace marocco
