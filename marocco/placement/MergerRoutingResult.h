#pragma once

#include <map>
#include <unordered_map>

#include "hal/Coordinate/L1.h"
#include "hal/Coordinate/Neuron.h"

namespace marocco {
namespace placement {

typedef std::unordered_map<
	HMF::Coordinate::HICANNOnWafer,
	std::map<HMF::Coordinate::NeuronBlockOnHICANN, HMF::Coordinate::DNCMergerOnHICANN> >
	MergerRoutingResult;

} // namespace placement
} // namespace marocco
