#pragma once

#include "hal/Coordinate/HICANN.h"

#include "marocco/placement/MergerRoutingResult.h"
#include "marocco/placement/internal/Result.h"

namespace marocco {
namespace placement {
class MergerTreeGraph;
namespace results {
class Placement;
} // namespace results
namespace parameters {
class MergerRouting;
} // namespace parameters
} // namespace placeent
namespace resource {
template <class T>
class Manager;
typedef Manager<HMF::Coordinate::HICANNGlobal> HICANNManager;
} // namespace resource
} // namespace marocco


namespace marocco {
namespace placement {

/**
 * @brief Map output of neurons to SPL1 repeaters and assign addresses.
 * @pre Neuron placement is available, see \c NeuronPlacement.
 */
class MergerRouting
{
public:
	/**
	 * @brief constructor for the Merger routing.
	 *
	 * @param [in] parameters: pymarocco parameters for the MergerRouting
	 * @param [in] denmem_assignment: placement of the neurons.
	 * @param [out] result: final MergerTree Configuration
	 * @param [in, mutable] res_mgr: used to load hardware constrains.
	 * @param [in] placement: placement of the neurons. required for L1AddressCalculations
	 * @param [in] bio_graph: bio_graph of the neural network.
	 * @param [in] address_parameters: pymarocco parameters for the L1AddressAssignment, used for
	 *correct Chain length calculations.
	 **/
	MergerRouting(
	    parameters::MergerRouting const& parameters,
	    internal::Result::denmem_assignment_type const& denmem_assignment,
	    MergerRoutingResult& result,
	    resource::HICANNManager const& res_mgr,
	    results::Placement const& placement,
	    graph_t const& bio_graph,
	    parameters::L1AddressAssignment const& address_parameters);

	void run(MergerTreeGraph const& graph, HMF::Coordinate::HICANNOnWafer const& hicann);

private:
	parameters::MergerRouting const& m_parameters;
	internal::Result::denmem_assignment_type const& m_denmem_assignment;
	MergerRoutingResult& m_result;
	resource::HICANNManager const& m_res_mgr;
	results::Placement const& m_placement;
	graph_t const& m_bio_graph;
	parameters::L1AddressAssignment const& m_address_parameters;
};

} // namespace placement
} // namespace marocco
