#include "marocco/placement/algorithms/byNeuronBlockEnumAndPopulationIDasc.h"

#include "marocco/Logger.h"
#include "marocco/placement/internal/OnNeuronBlock.h"

using namespace HMF::Coordinate;

namespace marocco {
namespace placement {
namespace algorithms {

void byNeuronBlockEnumAndPopulationIDasc::initialise()
{
	MAROCCO_INFO("using placement strategy: byNeuronBlockEnumAndPopulationIDasc");
	sort();
}

void byNeuronBlockEnumAndPopulationIDasc::sort_populations()
{
	MAROCCO_DEBUG("Sorting Populations");
	std::sort(
	    m_queue->begin(), m_queue->end(),
	    [&](NeuronPlacementRequest const& a, NeuronPlacementRequest const& b) -> bool {
		    return (*m_bio_graph)[a.population()]->id() > (*m_bio_graph)[b.population()]->id();
	    });
}

std::string byNeuronBlockEnumAndPopulationIDasc::get_name() const
{
	return "byNeuronBlockEnumAndPopulationIDasc";
}

} // namespace internal
} // namespace placement
} // namespace marocco
