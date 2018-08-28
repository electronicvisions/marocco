#pragma once

#include "marocco/placement/algorithms/byNeuronBlockEnumAndPopulationID.h"

namespace marocco {
namespace placement {
namespace algorithms {

/**
 * Placement of populations to a list of NeuronBlocksOnWafers.
 * Populations are placed in order of their ID onto NeuronBlocksOnWafers which
 * are sorted by their Enum
 *
 * which resutls in orders:
 * pop: 1 -> 2 -> 3 -> 4 -> ...  -> n-1 -> n
 * NB : H0 NB0 -> H0 NB1 -> H0 NB2 -> ... -> H0 NB7 -> H1 NB0 -> H1 NB2 -> ... -> H383 NB7
 *
 */
class byNeuronBlockEnumAndPopulationIDasc : public byNeuronBlockEnumAndPopulationID
{
public:
	void initialise() PYPP_OVERRIDE;

	std::string get_name() const PYPP_OVERRIDE;

protected:
	/**
	 * @brief Sorts Populations by id
	 */
	virtual void sort_populations() PYPP_OVERRIDE;

}; // byNeuronBlockEnumAndPopulationID

} // namespace internal
} // namespace placement
} // namespace marocco
