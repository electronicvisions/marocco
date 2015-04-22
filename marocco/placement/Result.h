#pragma once

#include "marocco/config.h"
#include "marocco/Result.h"
#include "marocco/PartialResult.h"
#include "marocco/placement/NeuronBlockMapping.h"
#include "marocco/placement/OutputBufferMapping.h"
#include "marocco/placement/PlacementMap.h"


namespace marocco {
namespace placement {

class LookupTable;

/** defines the hardware system type
 * and the result type for the neuron placement
 */
struct NeuronPlacementTypes
{
	typedef hardware_system_t hardware_type;
	typedef NeuronBlockMapping result_type;
};

/** class holding all neuron placements
 */
class NeuronPlacementResult :
	public PartialResult<NeuronPlacementTypes, HMF::Coordinate::HICANNGlobal, 0>
{
public:
	PlacementMap const& placement() const { return mPlacement; }
	PlacementMap&       placement()       { return mPlacement; }

private:
	PlacementMap mPlacement;
};

typedef NeuronPlacementResult neuron_placement_t;


struct OutputMappingTypes
{
	typedef hardware_system_t hardware_type;
	typedef OutputBufferMapping result_type;
};

class OutputMappingResult :
	public PartialResult<OutputMappingTypes, HMF::Coordinate::HICANNGlobal, 1>
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
