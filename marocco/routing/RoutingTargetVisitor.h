#pragma once

#include <limits>
#include <stdexcept>
#include <string>
#include <array>
#include <unordered_set>
#include <unordered_map>
#include <boost/graph/dijkstra_shortest_paths.hpp>

#include "marocco/routing/routing_graph.h"
#include "marocco/routing/L1Bus.h"
#include "marocco/routing/WaferRouting.h"
#include "hal/Coordinate/HMFGeometry.h"

namespace marocco {
namespace routing {

// signals early abort for successfull routing
class RoutingEarlyAbort : public std::exception
{
public:
	RoutingEarlyAbort(std::string const& msg) : mMsg(msg) {}

	virtual const char* what() const throw()
	{
		return mMsg.c_str();
	}

private:
	std::string mMsg;
};

// signals un unsecussfull routing, such that no route could be found
class RoutingFailed : public std::exception
{
public:
	RoutingFailed(std::string const& msg) : mMsg(msg) {}

	virtual const char* what() const throw()
	{
		return mMsg.c_str();
	}

private:
	std::string mMsg;
};

class RoutingTargetVisitor : public boost::dijkstra_visitor<>
{
public:
	typedef HMF::Coordinate::HICANNGlobal target_t;
	typedef routing_graph::vertex_descriptor vertex_t;
	typedef std::unordered_map<target_t, vertex_t> last_mile_t;
	typedef WaferRouting::Usage usage_t;

	RoutingTargetVisitor(
		std::vector<int> const& predecessor,
		usage_t& usage,
		std::unordered_set<target_t>& targets,
		last_mile_t& lastMile);

	template <class Vertex, class Graph>
	void finish_vertex(Vertex const v, Graph const& g);

protected:
	size_t index(int busid) const {
		if (busid>=128) {
			return 8+(busid/4)%8;
		}
		return (busid/4)%8;
	}

	/// keeps track of the vertical bus usage. There are only 14 reachable
	/// syndriver per class of vertical busses, but 16 candidates. So skip is
	/// bus is overused.
	usage_t& mUsage;

	/// reference to predecessor list. Contains the shortest path from v to
	/// source whenever `finish_vertex` is called.
	std::vector<int> const& mPredecessor;

	/// list of hicanns to be routed to. What HICANNs are remain within the set
	/// after routing can be considered unreachable
	std::unordered_set<target_t>& mTargets;

	/// a vector of buses which represent the terminal vertical bus from which to
	/// inject spikes into array. Routes can be reconstructed by following this
	/// vertices along the predecessor list.
	last_mile_t& mLastMile;

	/// remember already used crossbars, to avoid multiple switches per line
	std::unordered_map<vertex_t, vertex_t> mCrossbars;
};



template <class Vertex, class Graph>
void RoutingTargetVisitor::finish_vertex(Vertex const v, Graph const& g)
{
	if (g[v].getDirection() == L1Bus::Horizontal)
		return;

	auto const& bus = g[v];
	auto hicann = bus.hicann();
	auto it = mTargets.find(hicann);
	if (it != mTargets.end()) {
		// so we have a candidate

		// DEBUG only
		if (mLastMile.count(hicann)!=0) {
			throw std::runtime_error("entry for target exists already");
		}

		// see first whether we are already overused
		int const busid = bus.getBusId();
		if (mUsage[hicann.toHICANNOnWafer().id()][index(busid)]>=14) {
			return;
		}

		// no lets whether we violate some constraints
		Vertex cur=v, prev=mPredecessor[v];
		while (cur!=prev)
		{
			if (g[cur].getDirection() == L1Bus::Horizontal &&
				g[prev].getDirection() == L1Bus::Vertical)
			{
				// crossbar
				auto it = mCrossbars.find(cur);
				if (it!=mCrossbars.end() && it->second != prev) {
					// fail :-(
					return;
				}
			}

			prev = cur;
			cur = mPredecessor[cur];
		}

		cur=v;
		prev=mPredecessor[v];
		while (cur!=prev)
		{
			if (g[cur].getDirection() == L1Bus::Horizontal &&
				g[prev].getDirection() == L1Bus::Vertical)
			{
				// remember the crossbar
				mCrossbars[cur] = prev;
			}
			prev = cur;
			cur = mPredecessor[cur];
		}


		mLastMile[hicann] = v;
		mTargets.erase(it);
		mUsage[hicann.toHICANNOnWafer().id()][index(busid)]+=1;

		if (mTargets.empty()) {
			throw RoutingEarlyAbort("found all targets");
		}
	}
	//else {
		//// otherwise see whether east or west is a target
		//try {

			//if (bus.side() == HMF::Coordinate::left) {
				//hicann = hicann.west();
			//} else {
				//hicann = hicann.east();
			//}

			//it = mTargets.find(hicann);
			//if (it != mTargets.end()) {
				//// so we found a target
				//if (mLastMile.count(hicann)!=0) {
					//throw std::runtime_error("entry for target exists already");
				//}
				//mLastMile[hicann] = v;
				//mTargets.erase(it);
			//}
		//} catch (std::overflow_error const&) {
			//// reached the right or left bound of the Wafer, HICANNGlobal throws ... do
			//// noththing.
		//} catch (std::domain_error const&) {
			//// Is thrown whenever we use a invalid combination of X and Y, this
			//// happens because the wafer is round.
		//}
	//}
}

} // namespace routing
} // namespace marocco
