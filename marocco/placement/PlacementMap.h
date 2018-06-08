#pragma once

#include <marocco/graph.h>
#include <marocco/DistributedMap.h>
#include <marocco/assignment/Mapping.h>

namespace marocco {
namespace placement {

/**
 * The PlacementMap is a wrapped, distributed property map which maps Population
 * vertices onto Hardware.
 **/
typedef DistributedMap<graph_t, assignment::Mapping> PlacementMap;

} // namespace placement
} // namespace marocco
