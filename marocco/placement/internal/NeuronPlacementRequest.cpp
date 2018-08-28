#include "marocco/placement/internal/NeuronPlacementRequest.h"

#include "marocco/placement/internal/OnNeuronBlock.h"

namespace marocco {
namespace placement {
namespace internal {

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

bool NeuronPlacementRequest::operator==(NeuronPlacementRequest const& rhs) const
{
	// clang-format off
	return (this->mPopulationSlice == rhs.mPopulationSlice &&
	        this->mHwNeuronSize == rhs.mHwNeuronSize
	        );
	// clang-format off
}

bool NeuronPlacementRequest::operator!=(NeuronPlacementRequest const& rhs) const
{
	return !(*this == rhs);
}

size_t NeuronPlacementRequest::hash() const
{
	size_t hash = 0;
	boost::hash_combine(hash, this->mPopulationSlice);
	boost::hash_combine(hash, this->mHwNeuronSize);
	return hash;
}

size_t hash_value(NeuronPlacementRequest const& npr)
{
	return npr.hash();
}

} // namespace internal
} // namespace placement
} // namespace marocco
