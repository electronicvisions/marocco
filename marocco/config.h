#pragma once

// the Hardware stuff
#include "hal/Coordinate/geometry.h"
#include "hal/FPGAContainer.h"

#ifndef PYPLUSPLUS
#include "sthal/System.h"
#else
namespace sthal {
struct System;
struct HICANN;
}
#endif

#ifndef PYPLUSPLUS
#include "marocco/resource/HICANNManager.h"
#else
namespace marocco { namespace resource {
struct HICANNManager;
}}
#endif

namespace marocco {

typedef sthal::System hardware_system_t;

typedef marocco::resource::HICANNManager resource_manager_t;

} // marocco

namespace HMF {
class HICANNCollection;
}

// trait classes
template<typename T>
struct chip_type;

template<>
struct chip_type<sthal::System>
{
	// specifies the type of the underlying HW system
	typedef sthal::HICANN type;

	// corresponding coordinate type
	typedef HMF::Coordinate::HICANNGlobal index_type;

	// corresponding calibtic top level calibration data set
	typedef HMF::HICANNCollection calib_type;
};
