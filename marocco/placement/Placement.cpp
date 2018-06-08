#include "marocco/placement/Placement.h"

#include "marocco/Logger.h"
#include "marocco/placement/InputPlacement.h"
#include "marocco/placement/MergerRouting.h"
#include "marocco/placement/HICANNPlacement.h"
#include "marocco/placement/ReverseMapping.h"

using namespace HMF::Coordinate;
using HMF::HICANN::L1Address;

namespace marocco {
namespace placement {

std::shared_ptr<Placement::rev_map_t> Placement::getRevMapping() const
{
	if (!mRevMapping)
		throw std::runtime_error("reverse mapping not initialized");
	return mRevMapping;
}

DefaultPlacement::~DefaultPlacement()
{}

std::unique_ptr<typename Placement::result_type>
DefaultPlacement::run()
{
	std::unique_ptr<Result> result(new Result);

	HICANNPlacement hicannpl(
		mPyMarocco.placement,
		getGraph(), getHardware(),
		getManager(), getComm());
	set<0>(*result, hicannpl.run());

	auto output_mapping =
		std::unique_ptr<OutputMappingResult>(new OutputMappingResult());

	MergerRouting merger_routing(mPyMarocco, getGraph(), getHardware(), getManager());
	merger_routing.run(get<0>(*result), *output_mapping);

	// placement of externals, eg spike inputs
	InputPlacement input_placement(mPyMarocco,
	                               getGraph(), getHardware(), getManager());
	input_placement.run(get<0>(*result), *output_mapping);

	set<1>(*result, std::move(output_mapping));

	// create reverse mapping
	mRevMapping = std::make_shared<rev_map_t>(
		*result, getManager(), getGraph());

	return { std::move(result) };
}

} // namespace placement
} // namespace marocco
