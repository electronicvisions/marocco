#pragma once

#include <set>
#include <vector>

#include "hal/Coordinate/HICANN.h"
#include "hal/Coordinate/Neuron.h"
#include "marocco/config.h"
#include "marocco/placement/NeuronPlacementRequest.h"
#include "marocco/placement/NeuronPlacementResult.h"
#include "marocco/placement/PlacePopulations.h"
#include "marocco/placement/parameters/ManualPlacement.h"
#include "marocco/placement/parameters/NeuronPlacement.h"

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
		NeuronPlacementResult& result);

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
	std::vector<NeuronPlacementRequest> perform_manual_placement();

	void post_process(std::vector<PlacePopulations::result_type> const& placements);

	graph_t const& m_graph;
	parameters::NeuronPlacement const& m_parameters;
	parameters::ManualPlacement const& m_manual_placement;
	NeuronPlacementResult& m_result;
	/**
	 * @brief Working copy of denmem assignment.
	 * Comared to what is written to \c m_result at the end of #run(), this version
	 * contains all available HICANNs and their defects.  Only HICANNs that saw actual
	 * assignments are stored in \c m_result.
	 */
	NeuronPlacementResult::denmem_assignment_type m_denmem_assignment;
	std::set<HMF::Coordinate::HICANNOnWafer> m_used_hicanns;
};

} // namespace placement
} // namespace marocco
