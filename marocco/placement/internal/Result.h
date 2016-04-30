#pragma once

#include <unordered_map>
#include <vector>

#include "hal/Coordinate/HICANN.h"
#include "hal/Coordinate/Neuron.h"
#include "hal/Coordinate/typed_array.h"

#include "marocco/graph.h"
#include "marocco/placement/internal/L1AddressAssignment.h"
#include "marocco/placement/internal/OnNeuronBlock.h"

namespace marocco {
namespace placement {
namespace internal {

struct Result {
	typedef graph_t::vertex_descriptor vertex_descriptor;

	// TODO: Move into NeuronPlacement, as soon as this is not used outside placement anymore.
	typedef std::unordered_map<HMF::Coordinate::HICANNOnWafer,
	                           HMF::Coordinate::typed_array<internal::OnNeuronBlock,
	                                                        HMF::Coordinate::NeuronBlockOnHICANN> >
		denmem_assignment_type;

	typedef std::unordered_map<vertex_descriptor, std::vector<HMF::Coordinate::NeuronOnWafer> >
		primary_denmems_for_population_type;

	typedef std::unordered_map<HMF::Coordinate::HICANNOnWafer, internal::L1AddressAssignment>
		address_assignment_type;

	denmem_assignment_type denmem_assignment;
	primary_denmems_for_population_type primary_denmems_for_population;
	address_assignment_type address_assignment;
}; // Result

} // namespace internal
} // namespace placement
} // namespace marocco
