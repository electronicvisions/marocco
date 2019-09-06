#pragma once

#include "marocco/placement/algorithms/byNeuronBlockEnumAndPopulationID.h"

namespace marocco {
namespace placement {
namespace algorithms {

/**
 * Placement of populations to a list of NeuronBlockOnWafer
 * Populations are placed in order of their ID onto NeuronBlocksOnWafer which
 * are sorted by
 *  1. Their available denmems
 *  2. And if equal spiraling around the center
 */
class bySmallerNeuronBlockAndPopulationID : public byNeuronBlockEnumAndPopulationID
{
public:
	/**
	 * @brief set the init procedure to call sort()
	 */
	void initialise() PYPP_OVERRIDE;

protected:
	/**
	 * @brief Sorts NeuronBloks
	 * Will sort the provided neuron blocks to ensure that the smallest possible blocks
	 * are used first.
	 * A spiral ordering is applied for Neuron Blocks with same availability
	 */
	void sort_neuron_blocks() PYPP_OVERRIDE;

}; // bySmallerNeuronBlockAndPopulationID

} // namespace internal
} // namespace placement
} // namespace marocco
