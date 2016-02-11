#pragma once

#include "sthal/Wafer.h"
#include "marocco/coordinates/L1Route.h"
#include "marocco/coordinates/L1RouteTree.h"

namespace marocco {
namespace routing {

/**
 * @brief Configure sthal container to implement a given L1 route.
 * @note This does not check for configuration conflicts.
 * @todo Not all possible segments of \c L1Route are covered yet.
 */
void configure(sthal::Wafer& hw, L1Route const& route);

/**
 * @brief Configure sthal container to implement the given L1 routes.
 * @note This does not check for configuration conflicts.
 * @todo Not all possible segments of \c L1Route are covered yet.
 */
void configure(sthal::Wafer& hw, L1RouteTree const& tree);

} // namespace routing
} // namespace marocco
