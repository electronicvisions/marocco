#pragma once

#include "marocco/graph.h"
#include "marocco/placement/Result.h"
#include "pymarocco/PyMarocco.h"

namespace marocco {
namespace placement {

class MergerRouting
{
public:

	MergerRouting(
		pymarocco::PyMarocco& pymarocco,
		graph_t const& graph,
		hardware_system_t& hw,
		resource_manager_t const& mgr);

	void run(NeuronPlacementResult const& neuronpl,
			 OutputMappingResult& output_mapping);

private:
	void run(HMF::Coordinate::HICANNGlobal const& hicann,
			 NeuronBlockMapping const& nbmap,
			 OutputBufferMapping& local_output_mapping);

	graph_t const&            mGraph;
	hardware_system_t&        mHW;
	resource_manager_t const& mMgr;

	pymarocco::PyMarocco& mPyMarocco;
};

} // namespace placement
} // namespace marocco
