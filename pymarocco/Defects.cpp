#include "pymarocco/Defects.h"
#include <stdexcept>

namespace pymarocco {

Defects::Defects() :
	backend(Backend::None),
	path(),
	mHicanns(),
	mNeuronDefects(0),
	mBusDefects(0)
{}

float Defects::getNeuronDefect() const
{
	return mNeuronDefects;
}

void Defects::setNeuronDefect(float v)
{
	if (v<0.0 || v>1.0) {
		throw std::out_of_range("0<= rate <= 1");
	}
	mNeuronDefects = v;
}

float Defects::getBusDefect() const
{
	return mBusDefects;
}

void Defects::setBusDefect(float v)
{
	if (v<0.0 || v>1.0) {
		throw std::out_of_range("0<= rate <= 1");
	}
	mBusDefects = v;
}

void Defects::disable(HMF::Coordinate::HICANNGlobal id)
{
	mHicanns[id].reset();
}

void Defects::inject(HMF::Coordinate::HICANNGlobal id,
                     boost::shared_ptr<redman::resources::Hicann> res) {
	mHicanns[id] = res;
}

} // pymarocco
