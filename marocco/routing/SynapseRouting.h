#pragma once

#include <unordered_map>

#include "hal/Coordinate/HMFGeometry.h"
#include "marocco/placement/Result.h"
#include "marocco/routing/LocalRoute.h"
#include "marocco/routing/routing_graph.h"
#include "marocco/routing/SynapseRowSource.h"
#include "marocco/routing/Result.h"
#include "pymarocco/PyMarocco.h"

namespace marocco {
namespace routing {

class SynapseLoss;

class SynapseRouting
{
public:
	typedef SynapseRoutingResult::result_type Result;

	SynapseRouting(HMF::Coordinate::HICANNGlobal const& hicann,
				   boost::shared_ptr<SynapseLoss> const& sl,
				   pymarocco::PyMarocco const& pymarocco,
				   graph_t const& graph,
				   routing_graph const& routing_graph,
				   resource_manager_t const& mgr,
				   hardware_system_t& hw);

	HMF::Coordinate::HICANNGlobal const& hicann() const {
		return mHICANN;
	}

	void run(placement::Result const& placement,
			 std::vector<LocalRoute> const& route_list);

	/// sets synapse driver needs for given route
	void set(LocalRoute const& route, std::pair<std::size_t, std::size_t> const& need);

	/// returns synapse drivers needs for given route
	std::pair<std::size_t, std::size_t>
	get(LocalRoute const& route) const;

	Result const& getResult() const;
	Result&       getResult();

private:
	void handleSynapseLoss(
		LocalRoute const& local_route, placement::results::Placement const& neuron_placement);

	/// set all synapse decoder to the the 4-bit address disabling the synapse.
	void invalidateSynapseDecoder();

	/// temporariliy tag defect synapses with 4-bit weigh 1.
	void tagDefectSynapses();

	/// disable defect synapses by setting weight to 0 and decoder to 4-bit address disabling synapse
	void disableDefectSynapes();

	/// reference to pymarocco
	pymarocco::PyMarocco const& mPyMarocco;

	/// Coordinate of HICANN chip we are currently working on.
	HMF::Coordinate::HICANNGlobal const mHICANN;

	/// reference to sthal structure.
	hardware_system_t& mHW;

	/// reference to PyNN graph.
	graph_t const& mGraph;

	/// reference to L1 RoutingGraph.
	routing_graph const& mRoutingGraph;

	/// reference to resource manager
	resource_manager_t const& mManager;

	/// mapping of LocalRoutes to SynapseDriver requirements to realize ALL
	/// synapses.
	std::unordered_map<int, std::pair<std::size_t, std::size_t> > mNumSynapseDriver;

	Result mResult;

	boost::shared_ptr<SynapseLoss> mSynapseLoss;
};

} // namespace routing
} // namespace marocco
