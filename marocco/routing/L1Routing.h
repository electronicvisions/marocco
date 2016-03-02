#pragma once

#include "marocco/coordinates/L1Route.h"
#include "marocco/coordinates/L1RouteTree.h"
#include "marocco/routing/PathBundle.h"

namespace marocco {
namespace routing {

/**
 * @brief Create an \c L1Route from a sequence of routing graph vertices.
 * @note If you want sending repeaters to be configured you have to prepend a
 *       \c DNCMergerOnHICANN segment.
 * @throw InvalidRouteError if \c path does not represent a well-formed route.
 */
L1Route toL1Route(PathBundle::graph_type const& graph, PathBundle::path_type const& path);
L1RouteTree toL1RouteTree(PathBundle::graph_type const& graph, PathBundle const& bundle);

} // namespace routing
} // namespace marocco
