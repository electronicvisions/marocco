#pragma once

#include "marocco/assignment/PopulationSlice.h"

namespace marocco {
namespace placement {
namespace internal {

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

	assignment::PopulationSlice::value_type const& population() const;

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

	bool operator==(NeuronPlacementRequest const& rhs) const;
	bool operator!=(NeuronPlacementRequest const& rhs) const;

	size_t hash() const;

private:
	assignment::PopulationSlice mPopulationSlice;
	size_t mHwNeuronSize;
};

size_t hash_value(NeuronPlacementRequest const& npr);

} // namespace internal
} // namespace placement
} // namespace marocco

namespace std {
template <>
struct hash<marocco::placement::internal::NeuronPlacementRequest>
{
	size_t operator()(marocco::placement::internal::NeuronPlacementRequest const& npr) const
	{
		return hash_value(npr);
	}
};
} // namespace std
