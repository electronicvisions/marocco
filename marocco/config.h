#pragma once

// the Hardware stuff
#include "hal/Coordinate/geometry.h"
#include "hal/FPGAContainer.h"

#include "sthal/Wafer.h"

#ifndef PYPLUSPLUS
#include "marocco/resource/HICANNManager.h"
#else
namespace marocco { namespace resource {
struct HICANNManager;
}}
#endif

namespace marocco {

typedef marocco::resource::HICANNManager resource_manager_t;

} // marocco

namespace HMF {
class HICANNCollection;
}
