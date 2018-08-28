#pragma once

#include "marocco/placement/algorithms/PlacePopulationsBase.h"

namespace marocco {
namespace placement {
namespace algorithms {

/**
 * Placement of populations to a list of NeuronBlocksOnWafers.
 * Populations are placed in order of their ID onto NeuronBlocksOnWafers which
 * are sorted by their Enum
 *
 * which resutls in orders:
 * pop: n -> n-1 -> ... -> 3 -> 2 -> 1 -> 0
 * NB : H0 NB0 -> H0 NB1 -> H0 NB2 -> ... -> H0 NB7 -> H1 NB0 -> H1 NB2 -> ... -> H383 NB7
 *
 */
class byNeuronBlockEnumAndPopulationID : public PlacePopulationsBase
{
public:
	/**
	 * @brief set the init procedure to call sort()
	 */
	void initialise() PYPP_OVERRIDE;

	std::string get_name() const PYPP_OVERRIDE;

protected:
	/**
	 * @brief Sorts Neuron Blocks and Populations
	 */
	virtual void sort();

	/**
	 * @brief Sorts NeuronBloks
	 * Will sort the provided neuron blocks to ensure that the smallest possible blocks
	 * are used first.
	 * A spiral ordering is applied for Neuron Blocks with same availability
	 */
	virtual void sort_neuron_blocks();

	/**
	 * @brief Sorts Populations by id
	 */
	virtual void sort_populations();

}; // byNeuronBlockEnumAndPopulationID

} // namespace internal
} // namespace placement
} // namespace marocco
