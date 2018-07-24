#pragma once

#include "hal/Coordinate/Neuron.h"
#include "marocco/placement/internal/NeuronPlacementRequest.h"
#include "marocco/placement/internal/Result.h"

namespace marocco {
namespace placement {
namespace internal {

/** Placement of populations to a list of neuron blocks.
 */
class PlacePopulations
{
public:
	typedef HMF::Coordinate::NeuronOnWafer result_type;

	/**
	 * @param state
	 *        Container with hardware configuration.
	 *        Will be modified as placement progresses.
	 * @param neuron_blocks
	 *        Neuron blocks to use for placement.
	 *        Used-up blocks will be removed.
	 * @param queue
	 *        Population slices that are to be placed.
	 *        If placement fails, some requests may remain in this queue.
	 */
	PlacePopulations(
		graph_t const& graph,
		Result::denmem_assignment_type& state,
		std::vector<HMF::Coordinate::NeuronBlockOnWafer>& neuron_blocks,
		std::vector<NeuronPlacementRequest>& queue)
		: m_graph(graph), m_state(state), m_neuron_blocks(neuron_blocks), m_queue(queue)
	{
	}

	std::vector<result_type> const& run();
	/**
	 * Will sort the provided neuron blocks to ensure that the smallest possible blocks
	 * are used first.
	 */
	std::vector<result_type> const& sort_and_run();

private:
	void sort_neuron_blocks();
	bool place_one_population();
	OnNeuronBlock& on_neuron_block(HMF::Coordinate::NeuronBlockOnWafer const& nb);

	graph_t const& m_graph;
	Result::denmem_assignment_type& m_state;
	std::vector<HMF::Coordinate::NeuronBlockOnWafer>& m_neuron_blocks;
	std::vector<NeuronPlacementRequest>& m_queue;
	std::vector<result_type> m_result;
}; // PlacePopulations

} // namespace internal
} // namespace placement
} // namespace marocco
