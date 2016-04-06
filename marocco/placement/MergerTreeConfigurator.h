#pragma once

#include <array>

#include "sthal/Layer1.h"
#include "marocco/placement/MergerTreeGraph.h"
#include "marocco/placement/MergerTreeRouter.h"

namespace marocco {
namespace placement {

/**
 * @brief Configures the hardware to implement assignment between neuron blocks and DNC
 *        mergers found by `MergerTreeRouter`.
 * @note We do not need to handle defects here as there is only one possible path for each
 *       neuron block/DNC merger pair.  Because defects are considered by
 *       `MergerTreeRouter`, the presence of a pair tells us that there are no defects on
 *       this path.
 */
class MergerTreeConfigurator {
public:
	MergerTreeConfigurator(
		sthal::Layer1& layer1,
		MergerTreeGraph const& graph,
		MergerTreeRouter::result_type const& mapping);

	void run();

private:
	void connect(HMF::Coordinate::NeuronBlockOnHICANN const& nb, HMF::Coordinate::DNCMergerOnHICANN const& dnc_merger);
	void connect(MergerTreeGraph::vertex_descriptor src, MergerTreeGraph::vertex_descriptor dest);

	sthal::Layer1& m_layer1;
	MergerTreeGraph const& m_graph;
	MergerTreeRouter::result_type const& m_mapping;
	/// Keep track of mergers that were explicitly configured to use input from one side.
	/// As soon as we see them a second time, we know that we have to merge both inputs.
	std::array<bool, MergerTreeGraph::vertices_count()> m_configured;
}; // MergerTreeConfigurator

} // namespace placement
} // namespace marocco
