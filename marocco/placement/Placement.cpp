#include "marocco/placement/Placement.h"

#include "marocco/Logger.h"
#include "marocco/placement/InputPlacement.h"
#include "marocco/placement/MergerRouting.h"
#include "marocco/placement/HICANNPlacement.h"
#include "marocco/placement/LookupTable.h"

using namespace HMF::Coordinate;
using HMF::HICANN::L1Address;

namespace marocco {
namespace placement {

DefaultPlacement::~DefaultPlacement()
{}

std::unique_ptr<typename Placement::result_type>
DefaultPlacement::run()
{
	std::unique_ptr<Result> result(new Result);

	HICANNPlacement hicannpl(
		mPyMarocco.placement,
		getGraph(), getHardware(),
		getManager());
	hicannpl.run(result->neuron_placement);

	MergerRouting merger_routing(mPyMarocco, getGraph(), getHardware(), getManager());
	merger_routing.run(result->neuron_placement, result->output_mapping);

	// placement of externals, eg spike inputs
	InputPlacement input_placement(mPyMarocco,
	                               getGraph(), getHardware(), getManager());
	input_placement.run(result->neuron_placement, result->output_mapping);

	// create reverse mapping
	result->reverse_mapping = std::make_shared<LookupTable>(
		*result, getManager(), getGraph());

	return { std::move(result) };
}

} // namespace placement
} // namespace marocco
