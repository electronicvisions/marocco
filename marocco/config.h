#pragma once

// the Hardware stuff
#include "halco/common/geometry.h"
#include "hal/FPGAContainer.h"

#include "sthal/Wafer.h"

#ifndef PYPLUSPLUS
#include "marocco/resource/Manager.h"
#else
namespace marocco { namespace resource {
struct HICANNManager;
}}
#endif

namespace marocco {

typedef marocco::resource::HICANNManager resource_manager_t;
typedef marocco::resource::FPGAManager resource_fpga_manager_t;

} // marocco

namespace HMF {
class HICANNCollection;
}
