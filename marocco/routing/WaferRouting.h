#pragma once

#include <limits>
#include <vector>
#include <unordered_set>

#include "marocco/Algorithm.h"
#include "marocco/placement/Result.h"
#include "marocco/routing/Result.h"
#include "marocco/routing/WaferGraph.h"
#include "marocco/routing/SynapseLoss.h"
#include "marocco/routing/WeightMap.h"
#include "pymarocco/PyMarocco.h"

namespace marocco {
namespace routing {

/// Tracks usage of VLinesOnHICANN as insertion point.
class BusUsage
{
	typedef std::array<std::array<size_t, HMF::Coordinate::SynapseSwitchOnHICANN::v_period>,
	                   HMF::Coordinate::HICANNOnWafer::enum_type::size> array_type;

public:
	void increment(HMF::Coordinate::HICANNOnWafer const& hicann, L1Bus const& bus);

	size_t get(HMF::Coordinate::HICANNOnWafer const& hicann, L1Bus const& bus) const;

private:
	array_type mData;
};


struct WaferRouting :
	public Algorithm
{
public:
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

	std::unordered_set<HICANNGlobal> get_targets(
		placement::NeuronPlacementResult::primary_denmems_for_population_type const& revmap,
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
		HMF::Coordinate::DNCMergerOnHICANN const& source_dnc_merger,
		std::unordered_set<HICANNGlobal> const& unreachable,
		placement::NeuronPlacementResult const& neuron_mapping);

	// M E M B E R S
	pymarocco::PyMarocco& mPyMarocco;
	routing_graph& mRoutingGraph;
	WaferGraph mWaferGraph;
	boost::shared_ptr<SynapseLoss> const mSynapseLoss;

protected:
	BusUsage mUsage;
	placement::WaferL1AddressAssignment const* mAddressAssignment;
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
      mAddressAssignment(nullptr) {}

} // namespace routing
} // namespace marocco
