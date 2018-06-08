#pragma once

#include <array>
#include <memory>

#include "marocco/graph.h"
#include "marocco/placement/Result.h"

#include "pymarocco/PyMarocco.h"

namespace marocco {
namespace placement {

/**
 * @brief maps spike source populations and neuron blocks onto output buffer
 *
 * This class carries out the following task:
 *     place external spike sources onto dnc mergers
 *
 **/
struct InputPlacement
{
public:
	InputPlacement(pymarocco::PyMarocco& pymarocco,
				   graph_t const& graph,
				   hardware_system_t& hw,
				   resource_manager_t& mgr);

	void run(NeuronPlacementResult const& neuron_mapping,
			 OutputMappingResult& output_mapping);

private:
	void configureGbitLinks(HMF::Coordinate::HICANNGlobal const& hicann,
							OutputBufferMapping const& output_mapping);

	/** input spikes (bio) are inserted on free output buffers on target_hicann
	 */
	void insertInput(HMF::Coordinate::HICANNGlobal const& target_hicann,
					 OutputBufferMapping& om,
					 marocco::assignment::PopulationSlice& bio);

	graph_t const&           mGraph;
	hardware_system_t&       mHW;
	resource_manager_t&      mMgr;

	pymarocco::PyMarocco& mPyMarocco;
};

} // namespace placement
} // namespace marocco
