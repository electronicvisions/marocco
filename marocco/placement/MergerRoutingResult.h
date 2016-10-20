#pragma once

#include <map>
#include <unordered_map>

#include "hal/Coordinate/L1.h"
#include "hal/Coordinate/Neuron.h"
#include "hal/Coordinate/typed_array.h"

namespace marocco {
namespace placement {

typedef std::unordered_map<HMF::Coordinate::HICANNOnWafer,
                           HMF::Coordinate::typed_array<HMF::Coordinate::DNCMergerOnHICANN,
                                                        HMF::Coordinate::NeuronBlockOnHICANN> >
	MergerRoutingResult;

} // namespace placement
} // namespace marocco
