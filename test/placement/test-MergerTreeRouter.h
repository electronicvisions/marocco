#pragma once

#include "test/common.h"

#include "marocco/placement/MergerTreeRouter.h"

namespace marocco {
namespace placement {

class AMergerTreeRouter : public ::testing::Test
{
public:
	void add(HMF::Coordinate::NeuronBlockOnHICANN nb, size_t bio_neurons_count)
	{
		const internal::NeuronPlacementRequest dummy{
		    assignment::PopulationSlice{{}, 0 /*offset*/, 1 /*size*/}, 2 /*hw_neuron_size*/};
		auto& onb = neuron_block_mapping.at(nb);
		ASSERT_EQ(2, dummy.size());
		for (size_t ii = 0; ii < bio_neurons_count; ++ii) {
			ASSERT_NE(onb.end(), onb.add(dummy)) << ii;
		}
	}

	MergerTreeRouter build_router() {
		return {graph, neuron_block_mapping};
	}
	MergerTreeGraph graph;
	NeuronBlockMapping neuron_block_mapping;
}; // AMergerTreeRouter

} // placement
} // marocco
