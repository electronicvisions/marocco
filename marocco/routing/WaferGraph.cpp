#include "marocco/routing/WaferGraph.h"
#include "marocco/Logger.h"

using namespace HMF::Coordinate;

namespace marocco {
namespace routing {

size_t WaferGraph::horizontal_line_swap = 2;
size_t WaferGraph::vertical_line_swap = 2;

WaferGraph::WaferGraph(
	HMF::Coordinate::Wafer const& wafer,
	resource::HICANNManager const& mgr,
	pymarocco::PyMarocco const& pymarocco,
	routing_graph& graph) :
		mPyMarocco(pymarocco),
		mCoord(wafer),
		mGraph(graph),
		mMgr(mgr)
{
	setHorizontalSwap(mPyMarocco.routing.horizontal_line_swap);
	setVerticalSwap(mPyMarocco.routing.vertical_line_swap);

	// only build wafergraph for available HICANNs
	for(auto const& coord : mMgr.present())
	{
		if (mCoord==coord.toWafer()) {
			HICANNGraph hicann(coord, mPyMarocco, getRoutingGraph());
			mHICANN.insert(std::make_pair(coord, hicann));
		}
	}

	float const defect_rate = mPyMarocco.defects.getBusDefect();
	if (defect_rate > 0.) {
		warn(this) << "L1 defect rate " << defect_rate;
	}
	std::minstd_rand gen(42 /*seed*/);
	std::uniform_real_distribution<> dis(0, 1);

	// connect them to each other
	for(auto const& entry : mHICANN)
	{
		HICANNGlobal const& current = entry.first;
		HICANNGraph const& hicann = entry.second;

		// below
		try {
			HICANNGraph const& other = mHICANN.at(current.south());

			// connect busses with wire shift
			static size_t const max = VLineOnHICANN::end;
			if (defect_rate == 0.0) {
				for(size_t xx = 0; xx < max; ++xx)
				{
					VLineOnHICANN vline(xx);
					add_edge(hicann[vline], other[south(vline)], getRoutingGraph());
				}
			} else {
				for(size_t xx = 0; xx < max; ++xx)
				{
					if (dis(gen)>defect_rate) {
						VLineOnHICANN vline(xx);
						add_edge(hicann[vline], other[south(vline)], getRoutingGraph());
					}
				}
			}
		} catch (std::overflow_error const&) {
			// reached the bottom bound of the Wafer, HICANNGlobal throws ... do
			// noththing.
		} catch (std::domain_error const&) {
			// Is thrown whenever we use a invalid combination of X and Y, this
			// happens because the wafer is round.
		} catch (std::out_of_range const&) {
			// this happens whenever, the otherwise valid HICANN is not part of
			// the resource manager.
		}

		// to the right
		try {
			HICANNGraph const& other = mHICANN.at(current.east());

			static size_t const max = HLineOnHICANN::end;
			if (defect_rate == 0.0) {
				for(size_t yy = 0; yy < max; ++yy)
				{
					HLineOnHICANN hline(yy);
					add_edge(hicann[hline], other[east(hline)], getRoutingGraph());
				}
			} else {
				for(size_t yy = 0; yy < max; ++yy)
				{
					if (dis(gen)>defect_rate) {
						HLineOnHICANN hline(yy);
						add_edge(hicann[hline], other[east(hline)], getRoutingGraph());
					}
				}
			}
		} catch (std::overflow_error const&) {
			// reached the right bound of the Wafer, HICANNGlobal throws ... do
			// noththing.
		} catch (std::domain_error const&) {
			// Is thrown whenever we use a invalid combination of X and Y, this
			// happens because the wafer is round.
		} catch (std::out_of_range const&) {
			// this happens whenever, the otherwise valid HICANN is not part of
			// the resource manager.
		}
	}

	for (auto const& h: mMgr.present())
	{
		auto& hicann = mHICANN.at(h);
		auto const& defects = mMgr.get(h);

		auto const& hbuses = defects->hbuses();
		for (auto it=hbuses->begin_disabled(); it!=hbuses->end_disabled(); ++it)
		{
			clear_vertex(hicann[*it], getRoutingGraph());
			MAROCCO_INFO("Marked " << *it << " on " << h << " as defect/disabled");
		}

		auto const& vbuses = defects->vbuses();
		for (auto it=vbuses->begin_disabled(); it!=vbuses->end_disabled(); ++it)
		{
			clear_vertex(hicann[*it], getRoutingGraph());
			MAROCCO_INFO("Marked " << *it << " on " << h << " as defect/disabled");
		}
	}
}

Route::BusSegment
WaferGraph::getSendingL1(HICANNGlobal const& h,
						 unsigned const outbuffer /*0-7*/) const
{
	HICANNGraph const& hg = mHICANN.at(h);
	return hg.getSendingL1(outbuffer);
}

size_t WaferGraph::numL1Busses() const
{
	return boost::num_vertices(getRoutingGraph());
}

routing_graph& WaferGraph::getRoutingGraph()
{
	return mGraph;
}

routing_graph const& WaferGraph::getRoutingGraph() const
{
	return mGraph;
}

HLineOnHICANN WaferGraph::east(HLineOnHICANN const& hline) const
{
	return HLineOnHICANN((hline.value() + getHorizontalSwap()) % HLineOnHICANN::end);
}

VLineOnHICANN WaferGraph::south(VLineOnHICANN const& vline) const
{
	static size_t const end = VLineOnHICANN::end;
	if (vline.value() < end/2) {
		return VLineOnHICANN((vline.value() + end/2 - getVerticalSwap()) % (end/2));
	} else {
		return VLineOnHICANN(end/2 + (vline.value() - end/2 + getVerticalSwap()) % (end/2));
	}
}

size_t WaferGraph::getHorizontalSwap()
{
	return horizontal_line_swap;
}

void WaferGraph::setHorizontalSwap(size_t s)
{
	horizontal_line_swap = s;
}

size_t WaferGraph::getVerticalSwap()
{
	return vertical_line_swap;
}

void WaferGraph::setVerticalSwap(size_t s)
{
	vertical_line_swap = s;
}

} // namespace routing
} // namespace marocco
