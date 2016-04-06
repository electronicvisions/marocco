#include "marocco/placement/MergerRouting.h"

#include <unordered_map>

#include "hal/Coordinate/iter_all.h"
#include "marocco/Logger.h"
#include "marocco/placement/MergerTreeRouter.h"
#include "marocco/placement/MergerTreeConfigurator.h"
#include "marocco/util.h"
#include "marocco/util/chunked.h"

using namespace HMF::Coordinate;
using marocco::assignment::PopulationSlice;

namespace marocco {
namespace placement {

MergerRouting::MergerRouting(
	parameters::MergerRouting const& parameters,
	NeuronPlacementResult const& neuron_placement,
	MergerRoutingResult& result)
	: m_parameters(parameters), m_neuron_placement(neuron_placement), m_result(result)
{
}

void MergerRouting::run(MergerTreeGraph const& graph, HMF::Coordinate::HICANNOnWafer const& hicann)
{
	auto& merger_mapping = m_result[hicann];
	merger_mapping.clear();

	switch(m_parameters.strategy()) {
		case parameters::MergerRouting::Strategy::minimize_number_of_sending_repeaters: {
			// MergerTreeRouting tries to merge adjacent neuron blocks such that the
			// overall use of SPL1 outputs is minimized.  Every unused SPL1 output can
			// then be used for external input.

			MergerTreeRouter router(graph, m_neuron_placement.denmem_assignment().at(hicann));
			router.run();
			merger_mapping = router.result();
			break;
		} // case minSPL1
		case parameters::MergerRouting::Strategy::one_to_one: {
			// TODO: Instead of only reserving DNCMergerOnHICANN(7) for external input,
			// only insert neuron blocks that have neurons placed to them.
			merger_mapping[NeuronBlockOnHICANN(0)] = DNCMergerOnHICANN(0);
			merger_mapping[NeuronBlockOnHICANN(1)] = DNCMergerOnHICANN(1);
			merger_mapping[NeuronBlockOnHICANN(2)] = DNCMergerOnHICANN(2);
			merger_mapping[NeuronBlockOnHICANN(3)] = DNCMergerOnHICANN(3);
			merger_mapping[NeuronBlockOnHICANN(4)] = DNCMergerOnHICANN(4);
			merger_mapping[NeuronBlockOnHICANN(5)] = DNCMergerOnHICANN(5);
			merger_mapping[NeuronBlockOnHICANN(6)] = DNCMergerOnHICANN(6);
			break;
		} // case maxSPL1
		default:
			throw std::runtime_error("unknown merger tree strategy");
	} // switch merger tree strategy
}

} // namespace placement
} // namespace marocco
