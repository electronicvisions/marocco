#pragma once

#include <set>
#include <vector>

#include "hal/Coordinate/HICANN.h"
#include "hal/Coordinate/Neuron.h"

#include "marocco/config.h"
#include "marocco/placement/internal/NeuronPlacementRequest.h"
#include "marocco/placement/internal/PlacePopulations.h"
#include "marocco/placement/internal/Result.h"
#include "marocco/placement/parameters/ManualPlacement.h"
#include "marocco/placement/parameters/NeuronPlacement.h"
#include "marocco/placement/results/Placement.h"

namespace marocco {
namespace placement {

/**
 * Assign bio neurons to hardware neurons.
 * @note Takes user defined population placement into account.
 */
class NeuronPlacement
{
public:
	NeuronPlacement(
		graph_t const& graph,
		parameters::NeuronPlacement const& parameters,
		parameters::ManualPlacement const& manual_placement,
		results::Placement& result,
		internal::Result& internal);

	void add(HMF::Coordinate::HICANNOnWafer const& hicann);

	void add_defect(
	    HMF::Coordinate::HICANNOnWafer const& hicann,
	    HMF::Coordinate::NeuronOnHICANN const& neuron);

	void run();

private:
	void restrict_rightmost_neuron_blocks();
	void minimize_number_of_sending_repeaters();

	/**
	 * @brief Extract placement requests and run manual placement.
	 * @return Populations without manual placement information.
	 */
	std::vector<internal::NeuronPlacementRequest> perform_manual_placement();

	void post_process(std::vector<internal::PlacePopulations::result_type> const& placements);

	graph_t const& m_graph;
	parameters::NeuronPlacement const& m_parameters;
	parameters::ManualPlacement const& m_manual_placement;
	results::Placement& m_result;
	internal::Result& m_internal;
	/**
	 * @brief Working copy of denmem assignment.
	 * Comared to what is written to \c m_internal at the end of #run(), this version
	 * contains all available HICANNs and their defects.  Only HICANNs that saw actual
	 * assignments are stored in \c m_internal.
	 */
	internal::Result::denmem_assignment_type m_denmem_assignment;
	std::set<HMF::Coordinate::HICANNOnWafer> m_used_hicanns;
};

} // namespace placement
} // namespace marocco
