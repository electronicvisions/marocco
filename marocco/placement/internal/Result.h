#pragma once

#include <vector>
#include <boost/unordered_map.hpp>

#include "halco/hicann/v2/hicann.h"
#include "halco/hicann/v2/neuron.h"
#include "halco/common/typed_array.h"

#include "marocco/graph.h"

#ifndef PYPLUSPLUS
#include "marocco/placement/internal/L1AddressAssignment.h"
#include "marocco/placement/internal/OnNeuronBlock.h"
#else  //! PYPLUSPLUS
namespace marocco {
namespace placement {
namespace internal {
class L1AddressAssignment;
class OnNeuronBlock;
} // internal
} // placement
} // marocco
#endif //! PYPLUSPLUS

namespace marocco {
namespace placement {
namespace internal {

struct Result {
	typedef graph_t::vertex_descriptor vertex_descriptor;

	// TODO: Move into NeuronPlacement, as soon as this is not used outside placement anymore.
	typedef boost::unordered_map<
	    halco::hicann::v2::HICANNOnWafer,
	    halco::common::
	        typed_array<internal::OnNeuronBlock, halco::hicann::v2::NeuronBlockOnHICANN> >
	    denmem_assignment_type;

	typedef boost::unordered_map<vertex_descriptor, std::vector<halco::hicann::v2::NeuronOnWafer> >
	    primary_denmems_for_population_type;

	typedef boost::unordered_map<halco::hicann::v2::HICANNOnWafer, internal::L1AddressAssignment>
	    address_assignment_type;

	denmem_assignment_type denmem_assignment;
	primary_denmems_for_population_type primary_denmems_for_population;
	address_assignment_type address_assignment;
}; // Result

} // namespace internal
} // namespace placement
} // namespace marocco
