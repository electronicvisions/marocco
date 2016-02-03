#pragma once

#include "hal/Coordinate/HICANN.h"
#include "hal/Coordinate/L1.h"
#include "hal/Coordinate/typed_array.h"

#include "marocco/routing/routing_graph.h"
#include "pymarocco/PyMarocco.h"

namespace marocco {
namespace routing {

class HICANNGraph
{
	template <typename V, typename K>
	using typed_array = HMF::Coordinate::typed_array<V, K>;
	typedef HMF::Coordinate::VLineOnHICANN VLineOnHICANN;
	typedef HMF::Coordinate::HLineOnHICANN HLineOnHICANN;

public:
	typedef routing_graph::vertex_descriptor vertex_t;
	typedef typed_array<vertex_t, HLineOnHICANN> horizontal_busses_t;
	typedef typed_array<vertex_t, VLineOnHICANN> vertical_bussses_t;
	typedef typed_array<typed_array<bool, HLineOnHICANN>, VLineOnHICANN> switches_t;

	HICANNGraph(
		HMF::Coordinate::HICANNGlobal const& hicann,
		pymarocco::PyMarocco const& pymarocco,
		routing_graph& graph);

	vertex_t getSendingL1(HMF::Coordinate::DNCMergerOnHICANN const& m) const;

	vertex_t operator[] (HLineOnHICANN const& h) const;
	vertex_t operator[] (VLineOnHICANN const& v) const;

	// only for tests
	horizontal_busses_t const& getHorizontalBusses() const;
	vertical_bussses_t const&  getVerticalBusses() const;

private:
	bool exists(VLineOnHICANN const& v, HLineOnHICANN const& h) const;

	switches_t const& getSwitches() const;

	horizontal_busses_t mHorizontal;
	vertical_bussses_t mVertical;

	HMF::Coordinate::HICANNGlobal const mCoord;
	pymarocco::PyMarocco const& mPyMarocco;
	routing_graph& mGraph;

	switches_t const& mSwitches;
};

} // namespace routing
} // namespace marocco
