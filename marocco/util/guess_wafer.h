#pragma once

#include "halco/hicann/v2/wafer.h"
#include "marocco/resource/Manager.h"

namespace marocco {
namespace resource {
template <class T>
class Manager;
typedef Manager<halco::hicann::v2::HICANNGlobal> HICANNManager;
} // namespace resource

/**
 * @brief Returns coordinate of used wafer.
 * @note This is a temporary function to aid in the transition from global to wafer-local
 *       coordinates.  It assumes that only a single wafer is in use.
 */
halco::hicann::v2::Wafer guess_wafer(resource::HICANNManager const& mgr);

} // namespace marocco
