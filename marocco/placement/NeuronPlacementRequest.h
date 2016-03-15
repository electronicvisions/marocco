#pragma once

#include "marocco/assignment/PopulationSlice.h"

namespace marocco {
namespace placement {

/**
 * Represents part of a population that should be assigned to
 * a block of hardware neurons.
 *
 * @note: In many places throughout marocco it is assumed that the
 *        size of a bio neuron in terms of hardware neurons is
 *        divisible by two, i.e. a neuron placement vertically extends
 *        over the whole neuron block.
 */
class NeuronPlacementRequest {
public:
	NeuronPlacementRequest(assignment::PopulationSlice const& bio, size_t hw_neuron_size);

	typename assignment::PopulationSlice::value_type const& population() const;

	assignment::PopulationSlice& population_slice();
	assignment::PopulationSlice const& population_slice() const;

	/** Number of requested hardware neurons.
	 */
	size_t size() const;
	/** Size (in hardware neurons) of a single bio neuron.
	 */
	size_t neuron_size() const;
	/** Width (in hardware neurons) of a single bio neuron.
	 */
	size_t neuron_width() const;

private:
	assignment::PopulationSlice mPopulationSlice;
	size_t mHwNeuronSize;
};


} // namespace placement
} // namespace marocco
