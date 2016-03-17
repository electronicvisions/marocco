#pragma once

#include "marocco/AssociativeResult.h"
#include "marocco/Result.h"
#include "marocco/config.h"
#include "marocco/placement/NeuronPlacementResult.h"
#include "marocco/placement/OutputBufferMapping.h"


namespace marocco {
namespace placement {

class LookupTable;

typedef NeuronPlacementResult neuron_placement_t;

class OutputMappingResult
	: public AssociativeResult<HMF::Coordinate::HICANNOnWafer, OutputBufferMapping>
{};

typedef OutputMappingResult output_mapping_t;

struct Result : public BaseResult {
	NeuronPlacementResult neuron_placement;
	OutputMappingResult output_mapping;
	// reverse mapping for result data (only neuron address translation for now)
	std::shared_ptr<LookupTable> reverse_mapping;
};

} // namespace placement
} // namespace marocco
