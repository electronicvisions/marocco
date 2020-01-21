#include "marocco/placement/algorithms/byNeuronBlockEnumAndPopulationID.h"

#include "marocco/Logger.h"
#include "marocco/placement/internal/OnNeuronBlock.h"

using namespace halco::hicann::v2;

namespace marocco {
namespace placement {
namespace algorithms {

void byNeuronBlockEnumAndPopulationID::initialise()
{
	MAROCCO_INFO("using placement strategy: byNeuronBlockEnumAndPopulationID");
	sort();
}

void byNeuronBlockEnumAndPopulationID::sort()
{
	MAROCCO_DEBUG("Sorting NB and pops");
	sort_neuron_blocks();
	sort_populations();
}

void byNeuronBlockEnumAndPopulationID::sort_neuron_blocks()
{
	MAROCCO_DEBUG("Sorting NeuronBlocks");
	std::sort(m_neuron_blocks->begin(), m_neuron_blocks->end(), std::greater<>());
}

void byNeuronBlockEnumAndPopulationID::sort_populations()
{
	MAROCCO_DEBUG("Sorting Populations");
	std::sort(
	    m_queue->begin(), m_queue->end(),
	    [&](NeuronPlacementRequest const& a, NeuronPlacementRequest const& b) -> bool {
		    return (*m_bio_graph)[a.population()]->id() < (*m_bio_graph)[b.population()]->id();
	    });
}

} // namespace internal
} // namespace placement
} // namespace marocco
