#pragma once

#include "hal/Coordinate/Neuron.h"
#include "marocco/placement/NeuronPlacementRequest.h"

namespace marocco {
namespace placement {

class NeuronPlacementResult;
class OnNeuronBlock;

/** Placement of populations to a list of neuron blocks.
 */
class PlacePopulations
{
public:
	struct result_type
	{
		result_type(HMF::Coordinate::NeuronGlobal neuron_, NeuronPlacementRequest const& chunk_)
			: neuron(std::move(neuron_)), chunk(chunk_)
		{
		}

		HMF::Coordinate::NeuronGlobal neuron;
		NeuronPlacementRequest chunk;
	};

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
		NeuronPlacementResult& state,
		std::vector<HMF::Coordinate::NeuronBlockGlobal>& neuron_blocks,
		std::vector<NeuronPlacementRequest>& queue)
		: m_state(state), m_neuron_blocks(neuron_blocks), m_queue(queue)
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
	OnNeuronBlock& on_neuron_block(HMF::Coordinate::NeuronBlockGlobal const& nb);

	NeuronPlacementResult& m_state;
	std::vector<HMF::Coordinate::NeuronBlockGlobal>& m_neuron_blocks;
	std::vector<NeuronPlacementRequest>& m_queue;
	std::vector<result_type> m_result;
}; // PlacePopulations

} // namespace placement
} // namespace marocco
