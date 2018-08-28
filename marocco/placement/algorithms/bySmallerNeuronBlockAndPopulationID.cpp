#include "marocco/placement/algorithms/bySmallerNeuronBlockAndPopulationID.h"

#include "marocco/Logger.h"
#include "marocco/placement/internal/OnNeuronBlock.h"
#include "marocco/placement/internal/free_functions.h"
#include "marocco/util/spiral_ordering.h"

using namespace HMF::Coordinate;

namespace marocco {
namespace placement {
namespace algorithms {

void bySmallerNeuronBlockAndPopulationID::initialise()
{
	MAROCCO_INFO("using placement Strategy: bySmallerNeuronBlockAndPopulationID");
	sort();
}

void bySmallerNeuronBlockAndPopulationID::sort_neuron_blocks()
{
	// TODO: As neuron blocks are sorted by available space, we should maybe also sort the
	// population queue by population size to prevent fragmentation of the populations.
	// (Populations are sliced up if they don't fit into a single neuron block.)
	// At the moment auto-placed populations are sorted by their id in HICANNPlacement::run().
	//
	// NOTE by FP: the fragmentation is a huge problem in efficient routing of L1.
	//   many Neurons with same targets get torn apart, or targets get torn apart.
	//   especially the NB sorting by availability scatters the populations all over the wafer.
	//   in the end a lot more L1 buses are required, which leads to congestions.

	MAROCCO_DEBUG("Sorting neuron blocks");

	std::unordered_map<NeuronBlockOnWafer, size_t> available;
	for (auto const& nb : (*m_neuron_blocks)) {
		available.emplace(nb, internal::get_on_neuron_block_reference(*m_state, nb).available());
	}

	// Because pop_back() is more efficient for vectors, neuron blocks are sorted by size
	// in descending order but nevertheless processed small-to-big.
	// If neuron blocks on the same HICANN have the same size, they shall be processed from left to
	// right. Therefore, these neuron blocks are sorted from right to left.
	// Furthermore, HICANNs shall be used starting from the center, spiralling
	// outwards, if the neuron blocks have the same capacity. Hence, HICANNs
	// are sorted in reversed spiral ordering.
	spiral_ordering<HICANNOnWafer> ordering;
	std::sort(
	    m_neuron_blocks->begin(), m_neuron_blocks->end(),
	    [&available, &ordering](NeuronBlockOnWafer const& a, NeuronBlockOnWafer const& b) {
		    return (
		        (available[a] > available[b]) ||
		        (available[a] == available[b] &&
		         ((a.toHICANNOnWafer() == b.toHICANNOnWafer())
		              ? (a.toNeuronBlockOnHICANN() > b.toNeuronBlockOnHICANN())
		              : !ordering(a.toHICANNOnWafer(), b.toHICANNOnWafer()))));
	    });
}

std::string bySmallerNeuronBlockAndPopulationID::get_name() const
{
	return "bySmallerNeuronBlockAndPopulationID";
}

} // namespace internal
} // namespace placement
} // namespace marocco
