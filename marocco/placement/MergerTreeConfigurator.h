#pragma once

#include <array>

#include "sthal/Layer1.h"
#include "marocco/placement/MergerRoutingResult.h"
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
		MergerTreeGraph const& graph);

	void run(MergerRoutingResult::mapped_type const& mapping);
	void run(MergerTreeRouter::result_type const& mapping);

private:
	void connect(halco::hicann::v2::NeuronBlockOnHICANN const& nb, halco::hicann::v2::DNCMergerOnHICANN const& dnc_merger);
	void connect(MergerTreeGraph::vertex_descriptor src, MergerTreeGraph::vertex_descriptor dest);

	sthal::Layer1& m_layer1;
	MergerTreeGraph const& m_graph;
	/// Keep track of mergers that were explicitly configured to use input from one side.
	/// As soon as we see them a second time, we know that we have to merge both inputs.
	std::array<bool, MergerTreeGraph::vertices_count()> m_configured;
}; // MergerTreeConfigurator

} // namespace placement
} // namespace marocco
