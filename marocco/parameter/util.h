#pragma once

#include <vector>
#include <string>

#include "marocco/Logger.h"
#include "euter/typedcellparametervector.h"
#include "sthal/macros.h"

namespace marocco {
namespace parameter {

template<typename HardwareType>
std::string generateUID(
	HardwareType const& hw,
	int id,
	std::string suffix,
	char sep = '_')
{
	// construct hw entity name as follows <class name>_<id>_<index0>_<index1>....
	std::stringstream fname;

	// base name
	fname << typestring<typename std::decay<HardwareType>::type>();

	// append indices
	fname << sep << id << sep << hw.index();

	// append suffix
	fname << suffix;

	return fname.str();
}

template<CellType Type>
std::vector<typename CellTypeTraits<Type>::Parameters> const&
parameter_cast(CellParameterVector const& _vector)
{
	auto const& vec = dynamic_cast<TypedCellParameterVector<Type> const&>(_vector);
	return vec.parameters();
}

template<typename T>
class NeuronOnHICANNPropertyArray {
public:
	typedef HMF::Coordinate::NeuronOnHICANN neuron_coordinate;
	typedef T value_type;
	typedef std::array<std::array<value_type, neuron_coordinate::x_type::size>,
	                   neuron_coordinate::y_type::size> array_type;

	STHAL_ARRAY_OPERATOR(value_type, neuron_coordinate,
			return mArray[ii.y()][ii.x()];)
private:
	array_type mArray;
};

} // namespace parameter
} // namespace marocco
