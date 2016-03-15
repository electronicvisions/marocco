#include "marocco/placement/NeuronPlacementRequest.h"

#include "marocco/placement/OnNeuronBlock.h"

namespace marocco {
namespace placement {

NeuronPlacementRequest::NeuronPlacementRequest(assignment::PopulationSlice const& bio,
                                 size_t hw_neuron_size)
    : mPopulationSlice(bio), mHwNeuronSize(hw_neuron_size) {
	if (mHwNeuronSize % 2 != 0 || mHwNeuronSize == 0) {
		throw std::invalid_argument(
			"neuron size has to be even and non-null");
	}
}

typename assignment::PopulationSlice::value_type const&
NeuronPlacementRequest::population() const {
	return mPopulationSlice.population();
}

assignment::PopulationSlice& NeuronPlacementRequest::population_slice() {
	return mPopulationSlice;
}

assignment::PopulationSlice const& NeuronPlacementRequest::population_slice() const {
	return mPopulationSlice;
}

size_t NeuronPlacementRequest::size() const {
	return mPopulationSlice.size() * mHwNeuronSize;
}

size_t NeuronPlacementRequest::neuron_size() const {
	return mHwNeuronSize;
}

size_t NeuronPlacementRequest::neuron_width() const {
	// This is also implicitly assumed in many other places in marocco.
	static_assert(OnNeuronBlock::neuron_coordinate::y_type::size == 2,
	              "unexpected dimensions of neuron coordinate");

	return mHwNeuronSize / OnNeuronBlock::neuron_coordinate::y_type::size;
}

} // namespace placement
} // namespace marocco
