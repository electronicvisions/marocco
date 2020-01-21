#pragma once

#include <map>
#include <unordered_map>

#include "halco/hicann/v2/l1.h"
#include "halco/hicann/v2/neuron.h"
#include "halco/common/typed_array.h"

namespace marocco {
namespace placement {

typedef std::unordered_map<halco::hicann::v2::HICANNOnWafer,
                           halco::common::typed_array<halco::hicann::v2::DNCMergerOnHICANN,
                                                        halco::hicann::v2::NeuronBlockOnHICANN> >
	MergerRoutingResult;

} // namespace placement
} // namespace marocco
