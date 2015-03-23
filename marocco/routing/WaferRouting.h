#pragma once

#include <limits>
#include <vector>
#include <unordered_set>

#include "marocco/Algorithm.h"
#include "marocco/placement/Result.h"
#include "marocco/routing/Result.h"
#include "marocco/routing/SystemGraph.h"
#include "marocco/routing/WaferGraph.h"
#include "marocco/routing/SynapseLoss.h"
#include "marocco/routing/WeightMap.h"
#include "pymarocco/PyMarocco.h"

namespace marocco {
namespace routing {

struct WaferRouting :
	public Algorithm
{
public:
	typedef std::array<std::array<size_t, 16>, 384> Usage;

	template<typename ... Args>
	WaferRouting(HMF::Coordinate::Wafer const& wafer,
	             boost::shared_ptr<SynapseLoss> const& sl,
	             pymarocco::PyMarocco& pymarocco,
	             routing_graph& rgraph,
	             Args&& ... args);
	virtual ~WaferRouting();

	CrossbarRoutingResult run(placement::Result const& placement);

	routing_graph const& getRoutingGraph() const;

protected:
	// it doesn't actually remove the vertex, but only all edges from and to it.
	// this is important to not invalidate all existing vertex_descriptors.
	void clear_segment(Route::BusSegment const v);

private:
	friend class WaferRoutingTest;

	typedef HMF::Coordinate::HICANNGlobal HICANNGlobal;

	virtual Route::Segments
	allocateRoute(
		Route::BusSegment const source,
		std::unordered_set<HICANNGlobal> targets,
		std::unordered_set<HICANNGlobal>& unreachable) = 0;

	std::unordered_set<HICANNGlobal>
	get_targets(
		placement::PlacementMap const& revmap,
		std::vector<HardwareProjection> const& projections) const;

	/** Responsible for setting crossbar switches and repeaters too establish
	 * global L1 communication from HICANNs to HICANNs. However, without going
	 * the last mile over synapse driver, synapapses to neurons. This routing
	 * has to be established in the next step.
	 */
	void configureHardware(CrossbarRoutingResult const& routing);

	void configureCrossbar(
		L1Bus const& src,
		L1Bus const& trg);

	void configureSPL1Repeater(
		L1Bus const& src,
		L1Bus const& trg);

	void configureRepeater(
		L1Bus const& src,
		L1Bus const& trg,
		bool final);

	void handleSynapseLoss(
		HICANNGlobal const& source_hicann,
		std::vector<assignment::AddressMapping> const& sources,
		std::unordered_set<HICANNGlobal> const& unreachable,
		placement::NeuronPlacementResult const& neuron_mapping,
		placement::PlacementMap const& revmap);

	// M E M B E R S
	pymarocco::PyMarocco& mPyMarocco;
	routing_graph& mRoutingGraph;
	WaferGraph mWaferGraph;
	boost::shared_ptr<SynapseLoss> const mSynapseLoss;

protected:
	// usage of VLinesOnHICANN as insertion point.
	Usage mUsage;
	placement::OutputMappingResult const* mOutbMapping;
};


template <typename... Args>
WaferRouting::WaferRouting(HMF::Coordinate::Wafer const& wafer,
                           boost::shared_ptr<SynapseLoss> const& sl,
                           pymarocco::PyMarocco& pymarocco,
                           routing_graph& rgraph,
                           Args&&... args)
    : Algorithm(std::forward<Args>(args)...),
      mPyMarocco(pymarocco),
      mRoutingGraph(rgraph),
      mWaferGraph(wafer, getManager(), mPyMarocco, mRoutingGraph),
      mSynapseLoss(sl),
      mUsage(),
      mOutbMapping(0) {}

} // namespace routing
} // namespace marocco
