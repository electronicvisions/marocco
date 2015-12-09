#include "pymarocco/Placement.h"
#include <stdexcept>
#include "euter/population_view.h"

namespace pymarocco {

Placement::size_type const Placement::defaultNeuronSize = 4;
Placement::size_type const Placement::maxNeuronSize = HMF::Coordinate::NeuronOnNeuronBlock::enum_type::size;

Placement::Placement() :
	minSPL1(true),
	use_output_buffer7_for_dnc_input_and_bg_hack(false),
	mDefaultNeuronSize(defaultNeuronSize)
{}

void Placement::add(PopulationId const pop, Index const& first)
{
	add(pop, std::list<Index>{first}, mDefaultNeuronSize);
}

void Placement::add(PopulationId const pop, Index const& first, size_type size)
{
	add(pop, std::list<Index>{first}, size);
}

void Placement::add(PopulationId pop, std::list<Index> list)
{
	add(pop, list, mDefaultNeuronSize);
}

void Placement::add(PopulationId pop, std::list<Index> list, size_type size)
{
	checkNeuronSize(size);
	mPlacement[pop] = std::make_pair(list, size);
}

void Placement::add(PopulationId const pop, size_type size)
{
	checkNeuronSize(size);
	mPlacement[pop] = std::make_pair(List{}, size);
}

Placement::mapping_type::iterator
Placement::begin()
{
	return mPlacement.begin();
}

Placement::mapping_type::const_iterator
Placement::begin() const
{
	return mPlacement.begin();
}

Placement::mapping_type::const_iterator
Placement::cbegin() const
{
	return mPlacement.begin();
}

Placement::mapping_type::iterator
Placement::end()
{
	return mPlacement.end();
}

Placement::mapping_type::const_iterator  Placement::end() const
{
	return mPlacement.end();
}

Placement::mapping_type::const_iterator Placement::cend() const
{
	return mPlacement.end();
}

Placement::mapping_type const&
Placement::iter() const
{
	return mPlacement;
}

void Placement::setDefaultNeuronSize(size_type s)
{
	checkNeuronSize(s);
	mDefaultNeuronSize = s;
}

Placement::size_type
Placement::getDefaultNeuronSize() const
{
	return mDefaultNeuronSize;
}

void Placement::checkNeuronSize(size_type size)
{
	if ((size == 0) || ((size % 2) != 0) || (size > maxNeuronSize)) {
		throw std::runtime_error("only neuron sizes which are multiples of 2"
		                         " and not larger than 64 are allowed");
	}
}

} // pymarocco
