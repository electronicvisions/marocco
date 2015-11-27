#pragma once

#include <unordered_map>

#include "marocco/graph.h"
#include "marocco/assignment/NeuronBlockSlice.h"

namespace marocco {
namespace placement {

/// Maps population vertices onto neuron block slices.
typedef std::unordered_map<graph_t::vertex_descriptor, std::vector<assignment::NeuronBlockSlice> >
	PlacementMap;

} // namespace placement
} // namespace marocco
