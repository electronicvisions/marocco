#pragma once

#include <unordered_map>

#include "marocco/graph.h"
#include "hal/Coordinate/Neuron.h"

namespace marocco {
namespace placement {

/// Maps population vertices onto primary denmems.
/// Those can be used to look up all connected denmems via OnNeuronBlock.
typedef std::unordered_map<
	graph_t::vertex_descriptor,
	std::vector<HMF::Coordinate::NeuronOnWafer> >
	PlacementMap;

} // namespace placement
} // namespace marocco
