#include "HICANNGraph.h"

#include "hal/HICANN/Crossbar.h"
#include "hal/Coordinate/iter_all.h"

#include "marocco/Logger.h"

using namespace HMF::Coordinate;

namespace marocco {
namespace routing {

HICANNGraph::HICANNGraph(
	HMF::Coordinate::HICANNGlobal const& hicann,
	pymarocco::PyMarocco const& pymarocco,
	routing_graph& graph) :
		mCoord(hicann),
		mPyMarocco(pymarocco),
		mGraph(graph),
		mSwitches(pymarocco.routing.crossbar)
{
	// insert horizontal busses
	for (auto ii : iter_all<HLineOnHICANN>()) {
		vertex_t v = add_vertex(mGraph);
		mHorizontal[ii] = v;
		mGraph[v] = L1Bus(L1Bus::Horizontal, ii, mCoord);
	}

	// insert vertical busses
	for (auto ii : iter_all<VLineOnHICANN>()) {
		vertex_t v = add_vertex(mGraph);
		mVertical[ii] = v;
		mGraph[v] = L1Bus(L1Bus::Vertical, ii, mCoord);
	}


	float const defect_rate = mPyMarocco.defects.getBusDefect();

	std::minstd_rand gen(mCoord.toHICANNOnWafer().id());
	std::vector<std::pair<vertex_t, vertex_t>> switches;
	if (defect_rate == 0.0) {
		// insert edges
		for (auto yy : iter_all<HLineOnHICANN>()) {
			for (auto xx : iter_all<VLineOnHICANN>()) {
				if (exists(xx, yy)) {
					switches.push_back(std::make_pair(mHorizontal[yy], mVertical[xx]));
				}
			}
		}
	} else {
		warn(this) << "L1 defect rate " << defect_rate;

		std::uniform_real_distribution<> dis(0, 1);

		for (auto yy : iter_all<HLineOnHICANN>()) {
			for (auto xx : iter_all<VLineOnHICANN>()) {
				if (exists(xx, yy)) {
					if (dis(gen) > defect_rate) {
						switches.push_back(std::make_pair(mHorizontal[yy], mVertical[xx]));
					}
				}
			}
		}
	}

	if(mPyMarocco.routing.shuffle_crossbar_switches) {
		std::shuffle(switches.begin(), switches.end(), gen);
	}
	for (auto const& sw : switches) {
		add_edge(sw.first, sw.second, mGraph);
	}
}

HICANNGraph::vertex_t
HICANNGraph::getSendingL1(HMF::Coordinate::DNCMergerOnHICANN const& m) const
{
	HRepeaterOnHICANN repeater(m.toSendingRepeaterOnHICANN());
	return mHorizontal[repeater.toHLineOnHICANN()];
}

HICANNGraph::vertex_t
HICANNGraph::operator[] (HLineOnHICANN const& h) const
{
	return mHorizontal[h];
}

HICANNGraph::vertex_t
HICANNGraph::operator[] (VLineOnHICANN const& v) const
{
	return mVertical[v];
}

bool HICANNGraph::exists(VLineOnHICANN const& x, HLineOnHICANN const& y) const
{
	return mSwitches[x][y];
}

HICANNGraph::horizontal_busses_t const&
HICANNGraph::getHorizontalBusses() const
{
	return mHorizontal;
}

HICANNGraph::vertical_bussses_t const&
HICANNGraph::getVerticalBusses() const
{
	return mVertical;
}

} // namespace routing
} // namespace marocco
