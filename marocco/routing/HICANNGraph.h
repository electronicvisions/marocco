#pragma once
#include <array>

#include "marocco/routing/routing_graph.h"
#include "pymarocco/PyMarocco.h"

namespace marocco {
namespace routing {

class HICANNGraph
{
public:
	typedef HMF::Coordinate::VLineOnHICANN VLineOnHICANN;
	typedef HMF::Coordinate::HLineOnHICANN HLineOnHICANN;

	typedef routing_graph::vertex_descriptor vertex_t;
	typedef std::array<vertex_t, HLineOnHICANN::end> horizontal_busses_t;
	typedef std::array<vertex_t, VLineOnHICANN::end> vertical_bussses_t;
	typedef std::array<std::array<bool, HLineOnHICANN::end>,
			VLineOnHICANN::end> switches_t;

	HICANNGraph(
		HMF::Coordinate::HICANNGlobal const& hicann,
		pymarocco::PyMarocco const& pymarocco,
		routing_graph& graph);

	vertex_t getSendingL1(HMF::Coordinate::OutputBufferOnHICANN const& ob) const;

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
