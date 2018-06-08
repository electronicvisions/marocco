#pragma once

#include "marocco/config.h"
#include "marocco/PartialResult.h"
#include "marocco/ResultInterface.h"
#include "marocco/placement/NeuronBlockMapping.h"
#include "marocco/placement/OutputBufferMapping.h"
#include "marocco/placement/PlacementMap.h"


namespace marocco {
namespace placement {

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


// combine Results
class Result :
	public ResultInterface<NeuronPlacementResult, OutputMappingResult>
{
public:
	PlacementMap const& placement() const { return get<0>(*this).placement(); }
	PlacementMap&       placement()       { return get<0>(*this).placement(); }
};

} // namespace placement
} // namespace marocco
