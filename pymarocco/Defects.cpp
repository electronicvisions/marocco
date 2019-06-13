#include "pymarocco/Defects.h"
#include <stdexcept>

namespace pymarocco {

Defects::Defects() :
	backend(Backend::XML),
	path("/wang/data/calibration/brainscales/default"),
	mWafer(nullptr)
{}

void Defects::set(defects_t wafer) {
		mWafer = wafer;
}

Defects::defects_t Defects::wafer() {
		return mWafer;
}

Defects::defects_const_t Defects::wafer() const{
		return mWafer;
}

} // pymarocco
