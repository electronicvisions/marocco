#include "marocco/placement/internal/OnNeuronBlock.h"
#include "marocco/placement/internal/Result.h"

namespace marocco {
namespace placement {
namespace internal {

OnNeuronBlock& get_on_neuron_block_reference(
    Result::denmem_assignment_type& denmem_state, halco::hicann::v2::NeuronBlockOnWafer const& nb);

} // namespace internal
} // namespace placement
} // namespace marocco
