#pragma once

#include "marocco/placement/MergerRoutingResult.h"
#include "marocco/placement/MergerTreeGraph.h"
#include "marocco/placement/internal/Result.h"
#include "marocco/placement/parameters/MergerRouting.h"

namespace marocco {
namespace placement {

/**
 * @brief Map output of neurons to SPL1 repeaters and assign addresses.
 * @pre Neuron placement is available, see \c NeuronPlacement.
 */
class MergerRouting
{
public:
	MergerRouting(
		parameters::MergerRouting const& parameters,
		internal::Result::denmem_assignment_type const& denmem_assignment,
		MergerRoutingResult& result);

	void run(MergerTreeGraph const& graph, HMF::Coordinate::HICANNOnWafer const& hicann);

private:
	parameters::MergerRouting const& m_parameters;
	internal::Result::denmem_assignment_type const& m_denmem_assignment;
	MergerRoutingResult& m_result;
};

} // namespace placement
} // namespace marocco
