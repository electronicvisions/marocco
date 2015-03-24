#include "HICANNGraph.h"
#include "hal/HICANN/Crossbar.h"
#include "marocco/Logger.h"

using namespace HMF::Coordinate;

namespace marocco {
namespace routing {

namespace {
HICANNGraph::switches_t initSwitches()
{
	using HMF::HICANN::Crossbar;
	HICANNGraph::switches_t sw;
	for (size_t xx = 0; xx < VLineOnHICANN::end; ++xx) {
		for (size_t yy = 0; yy < HLineOnHICANN::end; ++yy) {
			sw.at(xx).at(yy) = Crossbar::exists(VLineOnHICANN(xx), HLineOnHICANN(yy));
		}
	}

	return sw;
}
} // namespace

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
	for (size_t ii = 0; ii < mHorizontal.size(); ++ii) {
		vertex_t v =  add_vertex(mGraph);
		mHorizontal[ii] = v;
		mGraph[v] = L1Bus(L1Bus::Horizontal, ii, mCoord);
	}

	// insert vertical busses
	for (size_t ii = 0; ii < mVertical.size(); ++ii) {
		vertex_t v = add_vertex(mGraph);
		mVertical[ii] = v;
		mGraph[v] = L1Bus(L1Bus::Vertical, ii, mCoord);
	}


	float const defect_rate = mPyMarocco.defects.getBusDefect();

	std::minstd_rand gen(mCoord.toHICANNOnWafer().id());
	std::vector<std::pair<vertex_t, vertex_t>> switches;
	if (defect_rate == 0.0) {
		// insert edges
		for (size_t yy = 0; yy < mHorizontal.size(); ++yy) {
			for (size_t xx = 0; xx < mVertical.size(); ++xx) {
				if (exists(VLineOnHICANN(xx), HLineOnHICANN(yy))) {
					switches.push_back(std::make_pair(mHorizontal[yy], mVertical[xx]));
				}
			}
		}

	} else {
		warn(this) << "L1 defect rate " << defect_rate;

		std::uniform_real_distribution<> dis(0, 1);

		for (size_t yy = 0; yy < mHorizontal.size(); ++yy) {
			for (size_t xx = 0; xx < mVertical.size(); ++xx) {
				if (exists(VLineOnHICANN(xx), HLineOnHICANN(yy))) {
					if (dis(gen)>defect_rate) {
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
HICANNGraph::getSendingL1(unsigned const outbuffer) const
{
	HRepeaterOnHICANN repeater(OutputBufferOnHICANN(outbuffer).toSendingRepeaterOnHICANN());
	assert(repeater.isSending());
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
