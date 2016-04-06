#pragma once

#include "marocco/placement/MergerRoutingResult.h"
#include "marocco/placement/MergerTreeGraph.h"
#include "marocco/placement/NeuronPlacementResult.h"
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
		NeuronPlacementResult const& neuron_placement,
		MergerRoutingResult& result);

	void run(MergerTreeGraph const& graph, HMF::Coordinate::HICANNOnWafer const& hicann);

private:
	parameters::MergerRouting const& m_parameters;
	NeuronPlacementResult const& m_neuron_placement;
	MergerRoutingResult& m_result;
};

} // namespace placement
} // namespace marocco
