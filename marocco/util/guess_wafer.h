#pragma once

#include "hal/Coordinate/Wafer.h"
#include "marocco/resource/HICANNManager.h"

namespace marocco {
namespace resource {
class HICANNManager;
} // namespace resource

/**
 * @brief Returns coordinate of used wafer.
 * @note This is a temporary function to aid in the transition from global to wafer-local
 *       coordinates.  It assumes that only a single wafer is in use.
 */
HMF::Coordinate::Wafer guess_wafer(resource::HICANNManager const& mgr);

} // namespace marocco
