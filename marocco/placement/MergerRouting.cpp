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
	internal::Result::denmem_assignment_type const& denmem_assignment,
	MergerRoutingResult& result)
	: m_parameters(parameters), m_denmem_assignment(denmem_assignment), m_result(result)
{
}

void MergerRouting::run(MergerTreeGraph const& graph, HMF::Coordinate::HICANNOnWafer const& hicann)
{
	auto& merger_mapping = m_result[hicann];

	for (auto const nb : iter_all<NeuronBlockOnHICANN>()) {
		merger_mapping[nb] = DNCMergerOnHICANN(nb);
	}

	switch(m_parameters.strategy()) {
		case parameters::MergerRouting::Strategy::minimize_number_of_sending_repeaters: {
			// MergerTreeRouting tries to merge adjacent neuron blocks such that the
			// overall use of SPL1 outputs is minimized.  Every unused SPL1 output can
			// then be used for external input.

			auto const& denmem_assignment = m_denmem_assignment.at(hicann);
			MergerTreeRouter router(graph, denmem_assignment);
			router.run();

			// If there is no entry in the merger mapping result, the corresponding neuron
			// block has not been merged with any other blocks and thus a 1-to-1
			// connection should be possible (only adjacent blocks are merged!).
			for (auto const& item : router.result()) {
				merger_mapping[item.first] = item.second;
			}

			break;
		}
		case parameters::MergerRouting::Strategy::one_to_one: {
			break;
		}
		default:
			throw std::runtime_error("unknown merger tree strategy");
	} // switch merger tree strategy
}

} // namespace placement
} // namespace marocco
