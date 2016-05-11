#include "pymarocco/Defects.h"
#include <stdexcept>

namespace pymarocco {

Defects::Defects() :
	backend(Backend::None),
	path(),
	mHicanns()
{}

void Defects::disable(HMF::Coordinate::HICANNGlobal id)
{
	mHicanns[id].reset();
}

void Defects::inject(HMF::Coordinate::HICANNGlobal id,
                     boost::shared_ptr<redman::resources::Hicann> res) {
	mHicanns[id] = res;
}

} // pymarocco
