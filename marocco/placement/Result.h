#pragma once

#include "marocco/config.h"
#include "marocco/Result.h"
#include "marocco/AssociativeResult.h"
#include "marocco/placement/NeuronBlockMapping.h"
#include "marocco/placement/OutputBufferMapping.h"
#include "marocco/placement/PlacementMap.h"


namespace marocco {
namespace placement {

class LookupTable;

/** class holding all neuron placements
 */
class NeuronPlacementResult
	: public AssociativeResult<HMF::Coordinate::HICANNOnWafer, NeuronBlockMapping>
{
public:
	PlacementMap const& placement() const { return mPlacement; }
	PlacementMap&       placement()       { return mPlacement; }

private:
	PlacementMap mPlacement;
};

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

	PlacementMap const& placement() const { return neuron_placement.placement(); }
	PlacementMap& placement() { return neuron_placement.placement(); }
};

} // namespace placement
} // namespace marocco
