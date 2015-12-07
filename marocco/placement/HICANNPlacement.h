#pragma once

#include <map>
#include <set>
#include <utility>
#include <vector>

#include "hal/Coordinate/Neuron.h"
#include "marocco/placement/NeuronPlacement.h"
#include "marocco/placement/PlacePopulations.h"
#include "marocco/placement/Result.h"
#include "marocco/test.h"

namespace pymarocco {
class Placement;
}

namespace marocco {
namespace placement {

/**
 * Assign bio neurons to hardware neurons.
 * @note Takes user defined population placement into account.
 */
class HICANNPlacement
{
public:
	HICANNPlacement(
		pymarocco::Placement const& pl,
		graph_t const& nn,
		hardware_system_t const& hw,
		resource_manager_t& mgr);

	/**
	 * @param res Output parameter used to store the result.
	 */
	void run(NeuronPlacementResult& res);

private:
	void disable_defect_neurons(NeuronPlacementResult& res);

	/** Extract placement requests and run manual placement.
	 *  @return Populations without manual placement information.
	 */
	std::vector<NeuronPlacement> manual_placement(NeuronPlacementResult& res);

	void post_process(
		NeuronPlacementResult& res, std::vector<PlacePopulations::result_type> const& placements);

	pymarocco::Placement const& mPyPlacement;
	graph_t const&            mGraph;
	hardware_system_t const&  mHW;
	resource_manager_t&       mMgr;

	FRIEND_TEST(HICANNPlacement, Basic);
};

} // namespace placement
} // namespace marocco
