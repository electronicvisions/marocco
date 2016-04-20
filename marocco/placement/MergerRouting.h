#pragma once

#include "marocco/graph.h"
#include "marocco/placement/Result.h"
#include "pymarocco/PyMarocco.h"

namespace marocco {
namespace placement {

/**
 * Map output of neurons to SPL1 repeaters and assign addresses.
 * @pre Neuron placement is available, see \c HICANNPlacement.
 */
class MergerRouting
{
public:

	MergerRouting(
		pymarocco::PyMarocco& pymarocco,
		graph_t const& graph,
		hardware_system_t& hw,
		resource_manager_t const& mgr);

	/**
	 * @param neuronpl Result of neuron placement step.
	 * @param output_mapping Output parameter used to store the result.
	 */
	void run(NeuronPlacementResult& neuron_placement,
			 OutputMappingResult& output_mapping);

private:
	void run(HMF::Coordinate::HICANNGlobal const& hicann,
			 NeuronPlacementResult& neuron_placement,
			 OutputBufferMapping& local_output_mapping);

	graph_t const&            mGraph;
	hardware_system_t&        mHW;
	resource_manager_t const& mMgr;

	pymarocco::PyMarocco& mPyMarocco;
};

} // namespace placement
} // namespace marocco
