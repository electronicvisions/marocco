#pragma once

#include "hal/Coordinate/Neuron.h"
#include "hal/Coordinate/typed_array.h"
#include "marocco/placement/OnNeuronBlock.h"

namespace marocco {
namespace placement {

/**
 * @brief Provides a mapping of populations to denmems of a HICANN by combining the
 *        OnNeuronBlock mappings of all neuron blocks.
 */
typedef HMF::Coordinate::typed_array<OnNeuronBlock, HMF::Coordinate::NeuronBlockOnHICANN>
    NeuronBlockMapping;

} // namespace placement
} // namespace marocco
