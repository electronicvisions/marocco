#pragma once

#include "marocco/AssociativeResult.h"
#include "marocco/Result.h"
#include "marocco/config.h"
#include "marocco/placement/MergerRoutingResult.h"
#include "marocco/placement/results/Placement.h"
#include "marocco/placement/internal/Result.h"


namespace marocco {
namespace placement {

class LookupTable;

struct Result : public BaseResult {
	Result(results::Placement const& result)
		: neuron_placement(result)
	{
	}

	results::Placement const& neuron_placement;
	internal::Result internal;
	MergerRoutingResult merger_routing;
	// reverse mapping for result data (only neuron address translation for now)
	std::shared_ptr<LookupTable> reverse_mapping;
};

} // namespace placement
} // namespace marocco
