#include "marocco/routing/WaferRoutingBackbone.h"

#include <sstream>
#include <list>

#include "marocco/routing/SynapseLoss.h"
#include "marocco/routing/WeightMap.h"
#include "marocco/placement/SpiralHICANNOrdering.h"
#include "marocco/placement/Result.h"
#include "marocco/routing/util.h"
#include "marocco/routing/SynapseDriverRequirements.h"
#include "marocco/Logger.h"
#include "hal/Coordinate/iter_all.h"


#define MAROCCO_SKIP_FAVOURITE_REPEATER(bus) \
	if ( bus .getBusId() == int(OutputBufferOnHICANN(3).repeater().line())) { \
		continue; \
	} \
	else if ( bus .getBusId() == int(OutputBufferOnHICANN(5).repeater().line())) { \
		continue; \
	}

using namespace HMF::Coordinate;
using marocco::placement::OutputMappingResult;

namespace marocco {
namespace routing {

namespace {

struct SortByX
{
	bool operator() (HICANNGlobal const& a, HICANNGlobal const& b)  const
	{
		if (a.x() != b.x()) {
			return (a.x() < b.x());
		} else {
			return a.toHICANNOnWafer().id() < b.toHICANNOnWafer().id();
		}
	}
};

struct SortByY
{
	bool operator() (HICANNGlobal const& a, HICANNGlobal const& b)  const
	{
		if (a.y() != b.y()) {
			return a.y() < b.y();
		} else {
			return a.toHICANNOnWafer().id() < b.toHICANNOnWafer().id();
		}
	}
};

class BackBoner
{
public:
	typedef routing_graph::vertex_descriptor Vertex;
	typedef std::unordered_map<HICANNGlobal, Route::BusSegment> LastMile;

	BackBoner(
		routing_graph const& rgraph,
		Route::BusSegment const source,
		std::unordered_set<HICANNGlobal>& targets,
		std::vector<routing_graph::vertex_descriptor>& predecessor,
		BusUsage const& usage,
		OutputMappingResult const& outbm)
		: mSource(source),
		  mAllTargets(targets),
		  mTargets(),
		  mPredecessor(predecessor),
		  mGraph(rgraph),
		  mUsage(usage),
		  mOutbufMapping(outbm)
	{
		for (auto const& trg : mAllTargets) {
			mTargets.insert(trg);
		}

		// efficient lookup for x coordinate
		std::set<int> xcoords;
		for (auto const& trg : mAllTargets) {
			mX[trg.x()].insert(trg);
			xcoords.insert(trg.x());
		}


		// now group them in pairs
		//HICANNGlobal const src = mGraph[mSource].hicann();
		//int last = -42;
		//for (int const x : xcoords)
		//{
			//if (last==x-1 && last!=int(src.x()) && x-1!=int(src.x())) {
				//mOption[x] = x-1;
				//mOption[x-1] = x;
				//last = -42;
			//} else {
				//last = x;
			//}
		//}
	}

	LastMile const&
	last_mile() const
	{
		return mLastMile;
	}


	void find_path()
	{
		if (mAllTargets.empty()) {
			return;
		}

		size_t const leftMost = mTargets.begin()->x();
		size_t const rightMost = mTargets.rbegin()->x();

		HICANNGlobal const& cur = mGraph[mSource].hicann();

		walkHorizontal(leftMost,  mSource, -1);
		walkHorizontal(rightMost, mSource,  1);

		auto xit = mX.find(cur.x());
		if (xit != mX.end())
		{
			// start to walk vertically
			walkVertical(mSource);
		}

		// remove reached targets from targets list
		for (auto const& entry : mLastMile) {
			mAllTargets.erase(entry.first);
		}
	}

private:

	// dir +1 => right and dir -1 => left
	bool walkHorizontal(
		size_t const border,
		Vertex const v /*horizontal*/,
		int const dir)
	{
		HICANNGlobal const cur = mGraph[v].hicann();
		if ((dir==1 && border<cur.x()) || (dir==-1 && border>cur.x())) {
			// return `true`, we successfully reached the ends without
			// encountering any error.
			return true;
		}

		routing_graph::adjacency_iterator it, eit;

		// find next bus
		int vn = -1;
		std::tie(it, eit) = boost::adjacent_vertices(v, mGraph);
		for (; it!=eit; ++it)
		{
			L1Bus const& adjBus = mGraph[*it];
			HICANNGlobal const& adjChip = adjBus.hicann();
			if (adjBus.getDirection()==L1Bus::Horizontal && adjChip.x()==cur.x()+dir)
			{
				//MAROCCO_SKIP_FAVOURITE_REPEATER(adjBus)
				if (blocksUsedOutputBuffer(adjBus)) {
					continue;
				}
				vn = *it;
				break;
			}
		}

		bool success = false;
		if (vn!=-1) {
			HICANNGlobal next = mGraph[vn].hicann();

			// keep on walking
			success = walkHorizontal(border, vn, dir);

			auto xit = mX.find(next.x());
			if (xit != mX.end())
			{
				walkVertical(vn);
			}

			mPredecessor[vn] = v;
		} else {
			MAROCCO_INFO("we are stuck, try detour " << v);
			success = detour(border, v, dir);
		}

		// recursive detour
		//if (!success) {
			//MAROCCO_INFO("we are stuck, try detour " << v);
			//success = detour(border, v, dir);
		//}

		return success;
	}

	// try to detour vertically
	bool detour(
		size_t const border,
		Vertex const v /*horizontal*/,
		int const dir)
	{
		HICANNGlobal const cur = mGraph[v].hicann();

		// first get all vertical candidates
		std::vector<Vertex> candidates;
		routing_graph::adjacency_iterator it, eit;
		std::tie(it, eit) = boost::adjacent_vertices(v, mGraph);
		for (; it!=eit; ++it)
		{
			L1Bus const& adjBus = mGraph[*it];
			if (adjBus.getDirection()==L1Bus::Vertical)
			{
				candidates.push_back(*it);
			}
		}

		// iterate verically and find crossbar which leads to greatest success
		int bestcand=-1, bestcbv=-1, bestcbh=-1, bestdir=-1, max=1;
		for (auto const& cand : candidates)
		{
			// try both, up and downwards
			for (auto const downward : { -1, 1 })
			{
				HICANNGlobal _cur = cur;
				Vertex _v = cand;
				while (true)
				{
					bool success = false;
					std::tie(it, eit) = boost::adjacent_vertices(_v, mGraph);
					for (; it!=eit; ++it)
					{
						L1Bus const& adjBus = mGraph[*it];
						HICANNGlobal const& adjChip = adjBus.hicann();
						if (adjBus.getDirection()==L1Bus::Vertical && adjChip.y()==_cur.y()+downward)
						{
							// test all reachable horizontal busses
							routing_graph::adjacency_iterator _it, _eit;
							std::tie(_it, _eit) = boost::adjacent_vertices(*it, mGraph);
							for (; _it!=_eit; ++_it)
							{
								L1Bus const& _adjBus = mGraph[*_it];
								if (_adjBus.getDirection()==L1Bus::Horizontal)
								{
									//MAROCCO_SKIP_FAVOURITE_REPEATER(_adjBus)
									if (blocksUsedOutputBuffer(_adjBus)) {
										continue;
									}

									// now count how far we can reach horizontally
									int const cnt = walkHorizontalCount(border, *_it, dir);
									// we have better solution
									if (cnt > max) {
										bestcbv = *it;
										bestcbh = *_it;
										bestcand = cand;
										bestdir = downward;
										max = cnt;

										//MAROCCO_INFO("besth: " << bestcbh << " bestv: " << bestcbv
													 //<< " bestdir: " << bestdir << " " << _adjBus.hicann());
									}
								} else {
									//MAROCCO_INFO("INNER VERTICAL");
								}
							}

							// we found the vertical continuation
							_v = *it;
							_cur = adjChip;
							success = true;
							break;
						} else {
							//MAROCCO_INFO("LALALA " << _cur.y() << " " << adjChip.y() << " " << downward)
						}
					}
					if (!success) {
						break;
					}
				}
			}
		}

		// nothing to do
		if (bestcbh==-1) {
			MAROCCO_DEBUG("no detour found " << candidates.size());
			return false;
		}

		// now, we have a winner, we now need to walk the detour
		HICANNGlobal _cur = cur;
		Vertex _v = bestcand;
		std::vector<Vertex> memorize;
		while (true)
		{
			memorize.push_back(_v);
			std::tie(it, eit) = boost::adjacent_vertices(_v, mGraph);
			for (; it!=eit; ++it)
			{
				L1Bus const& adjBus = mGraph[*it];
				HICANNGlobal const& adjChip = adjBus.hicann();
				MAROCCO_DEBUG("vertex " << int(*it) << " bestdir: " << bestdir
							  << " besth: " << bestcbh << " bestv: " << bestcbv);
				if (adjBus.getDirection()==L1Bus::Vertical && adjChip.y()==_cur.y()+bestdir)
				{
					mPredecessor[*it] = _v;

					if (*it == Vertex(bestcbv)) {
						// we reached the point
						walkHorizontal(border, bestcbh, dir);
						mPredecessor[*it] = _v;
						mPredecessor[bestcbh] = bestcbv; // second detour crossbar
						// this line is now redundant with memorize below
						mPredecessor[bestcand] = v;      // first detour crossbar
						for(size_t ii=0; ii+1<memorize.size(); ++ii) {
							mPredecessor[memorize[ii+1]] = memorize[ii];
						}
						MAROCCO_INFO("Successfull detour");
						return true;
					}

					// we found the vertical continuation
					_v = *it;
					_cur = adjChip;
					break;
				}
			}
		}
		return false;
	}

	size_t walkHorizontalCount(
		size_t const border,
		Vertex v,
		int const dir)
	{
		HICANNGlobal cur= mGraph[v].hicann();
		if ((dir==1 && border<cur.x()) || (dir==-1 && border>cur.x())) {
			return 0;
		}

		routing_graph::adjacency_iterator it, eit;

		// find next bus
		int vn = -1;
		std::tie(it, eit) = boost::adjacent_vertices(v, mGraph);
		for (; it!=eit; ++it)
		{
			L1Bus const& adjBus = mGraph[*it];
			HICANNGlobal const& adjChip = adjBus.hicann();
			if (adjBus.getDirection()==L1Bus::Horizontal && adjChip.x()==cur.x()+dir)
			{
				//MAROCCO_SKIP_FAVOURITE_REPEATER(adjBus)
				if (blocksUsedOutputBuffer(adjBus)) {
					continue;
				}

				vn = *it;
				break;
			}
		}

		if (vn!=-1) {
			// keep on walking
			return walkHorizontalCount(border, vn, dir) + 1;
		} else {
			return 0;
		}
	}


	void walkVertical(Vertex const v /*horizontal*/)
	{
		routing_graph::adjacency_iterator it, eit;

		// check for optional inserts
		int xopt = -1, vopt = -1;
		HICANNGlobal hopt;
		HICANNGlobal const cur = mGraph[v].hicann();
		//if (mOption.find(cur.x()) != mOption.end()) {
			//int const _xopt = mOption.at(cur.x());
			//try {
				//hopt = HICANNGlobal(X(_xopt), cur.y());
			//} catch (std::domain_error const&) {
				//// invalid combination of X and Y
				//return;
			//}
			//std::tie(it, eit) = boost::adjacent_vertices(v, mGraph);
			//for (; it!=eit; ++it)
			//{
				//HICANNGlobal const& adjChip = mGraph[*it].hicann();
				//if (adjChip == hopt) {
					//vopt = *it;
					//break;
				//}
			//}
			//if (vopt != -1) {
				//xopt = _xopt;
			//}

			//mOption.erase(_xopt);
			//mOption.erase(cur.x());
		//}

		std::vector<Vertex> candidates;
		if (xopt!=-1) {
			bool const opt_is_right = int(cur.x())<xopt ? true : false;
			add_candidate(v,    !opt_is_right, candidates);
			add_candidate(vopt,  opt_is_right, candidates);
		} else {
			std::tie(it, eit) = boost::adjacent_vertices(v, mGraph);
			for (; it!=eit; ++it)
			{
				L1Bus const& adjBus = mGraph[*it];
				if (adjBus.getDirection()==L1Bus::Vertical)
				{
					candidates.push_back(*it);
				}
			}
		}

#if !defined(MAROCCO_NDEBUG)
		if (candidates.size()>8) {
			throw std::runtime_error("too many candidates");
		}
#endif // MAROCCO_NDEBUG


		auto targets = mX.at(cur.x());
		if (xopt!=-1) {
			size_t n = targets.size();
			for (auto const& trg : mX.at(xopt)) {
				targets.insert(trg);
			}
#if !defined(MAROCCO_NDEBUG)
			if (mX.at(xopt).size() && n == targets.size()) {
				throw std::runtime_error("WTF");
			}
#endif // MAROCCO_NDEBUG
		}

		size_t topMost = targets.begin()->y();
		size_t botMost = targets.rbegin()->y();

		// try all candidates to greedily pick best option
		int best=-1, max=0;
		for (Vertex const cand : candidates)
		{
			int const cnt = walkVerticalCount(targets, cand);
			if (cnt>max) {
				max = cnt;
				best = cand;
			}
		}

		// now use the best option
		if (best!=-1)
		{
			mX.erase(xopt);
			mX.erase(cur.x());

			walkVerticalUse_(targets, best, topMost,  1);
			walkVerticalUse_(targets, best, botMost, -1);

			// set crossbar
			if (xopt!=-1) {
				auto const& chip = mGraph[best].hicann();
				if (chip==cur) {
					// vertical bus is on current hicann
					mPredecessor[best] = v;
				} else {
					// vertical bus is on adjacent hicann
					mPredecessor[best] = vopt;
					mPredecessor[vopt] = v;
				}
			} else {
				// vertical bus is on current hicann
				mPredecessor[best] = v;
			}

			if (targets.find(cur) != targets.end()) {
				mLastMile[cur] = best;
			}

			if (xopt!=-1 && targets.find(hopt) != targets.end()) {
				mLastMile[hopt] = best;
			}
		}
	}

	/*
	  Sebastian Jeltsch:
	  A Scalable Workflow for a Configurable Neuromorphic Platform

	  http://archiv.ub.uni-heidelberg.de/volltextserver/17190/

	  When a target chip is encountered the score is increment by two,
	  if less than 12 other routes are already competing for the same set of
	  synapse drivers, otherwise by one. The scoring is useful to reduce the
	  number of competing routes because up to 16 vertical buses share 14
	  synapse drivers, see Section 1.5.3.

	  Comment: The implemented score is greater by 1 w.r.t. the
	  description in the thesis.
	*/
	size_t scoreVerticalCount(size_t vertical_bus_usage) const {
		return vertical_bus_usage < 12 ? 3 : 2;
	}

	size_t walkVerticalCount(
		std::set<HICANNGlobal, SortByY> const& targets,
		Vertex v) const
	{
		L1Bus const& bus = mGraph[v];
		HICANNGlobal const cur = bus.hicann();

		size_t cnt=0;
		if (targets.find(cur) != targets.end()) {
			cnt += scoreVerticalCount(mUsage.get(cur.toHICANNOnWafer(), bus));
		}

		size_t topMost = targets.begin()->y();
		size_t botMost = targets.rbegin()->y();

		//MAROCCO_INFO("current: " << cur.y() << " topMost: " << topMost << " botMost: " << botMost);

		cnt+=walkVerticalCount_(targets, v, topMost, 1);
		cnt+=walkVerticalCount_(targets, v, botMost, -1);

		return cnt;
	}

	// dir +1 => up and dir -1 => down
	size_t walkVerticalCount_(
		std::set<HICANNGlobal, SortByY> const& targets,
		Vertex v,
		size_t const border,
		int const dir) const
	{
#if !defined(MAROCCO_NDEBUG)
		if (!(dir==-1 || dir==1)) {
			throw std::runtime_error("invalid direction");
		}
#endif // MAROCCO_NDEBUG

		L1Bus const& bus = mGraph[v];
		HICANNGlobal const cur = bus.hicann();

		// check if we reached the outer most target
		if ((dir==1 && border>cur.y()) || (dir==-1 && border<cur.y())) {
			return 0;
		}

		size_t cnt=0;
		if (targets.find(cur) != targets.end()) {
			cnt += scoreVerticalCount(mUsage.get(cur.toHICANNOnWafer(), bus));
		}

		// find next vertical bus in `dir` direction
		int vn = -1;
		routing_graph::adjacency_iterator it, eit;
		std::tie(it, eit) = boost::adjacent_vertices(v, mGraph);
		for (; it!=eit; ++it)
		{
			L1Bus const& adjBus = mGraph[*it];
			HICANNGlobal const& adjChip = adjBus.hicann();
			if (adjBus.getDirection()==L1Bus::Vertical && adjChip.y()+dir == cur.y())
			{
				vn = *it;
				break;
			}
		}

		// if we found next vertical bus, continue recursion
		if (vn != -1) {
			cnt += walkVerticalCount_(targets, vn, border, dir);
		}

		return cnt;
	}


	void walkVerticalUse_(
		std::set<HICANNGlobal, SortByY> const& targets,
		Vertex v,
		size_t const border,
		int const dir)
	{
#if !defined(MAROCCO_NDEBUG)
		if (!(dir==-1 || dir==1)) {
			throw std::runtime_error("invalid direction");
		}
#endif // MAROCCO_NDEBUG

		HICANNGlobal const cur = mGraph[v].hicann();
		// check if we reached the outer most target
		if ((dir==1 && border>cur.y()) || (dir==-1 && border<cur.y())) {
			return;
		}

		// find next vertical bus in `dir` direction
		int vn = -1;
		routing_graph::adjacency_iterator it, eit;
		std::tie(it, eit) = boost::adjacent_vertices(v, mGraph);
		for (; it!=eit; ++it)
		{
			L1Bus const& adjBus = mGraph[*it];
			HICANNGlobal const& adjChip = adjBus.hicann();
			if (adjBus.getDirection()==L1Bus::Vertical && adjChip.y()+dir == cur.y())
			{
				vn = *it;
				break;
			}
		}

		// if we found next vertical bus, continue recursion
		if (vn != -1) {
			HICANNGlobal const next = mGraph[vn].hicann();
			if (targets.find(next) != targets.end()) {
				mLastMile[next] = vn;
			}

			// left
			try {
				HICANNGlobal const left = HICANNGlobal(X(next.x()-1), next.y(), next.toWafer());
				if (targets.find(left) != targets.end()) {
					mLastMile[left] = vn;
				}
			} catch (...) {}

			// right
			try {
				HICANNGlobal const right = HICANNGlobal(X(next.x()+1), next.y(), next.toWafer());
				if (targets.find(right) != targets.end()) {
					mLastMile[right] = vn;
				}
			} catch (...) {}

			// TODO: this is not optimal, because more buses might be allocated
			// then necessary, if outer targets are not available.
			mPredecessor[vn] = v;
			walkVerticalUse_(targets, vn, border, dir);
		}
	}

	void add_candidate(Vertex const v, bool const right, std::vector<Vertex>& candidates) const
	{
		routing_graph::adjacency_iterator it, eit;
		std::tie(it, eit) = boost::adjacent_vertices(v, mGraph);
		for (; it!=eit; ++it)
		{
			L1Bus const& adjBus = mGraph[*it];
			if (right) {
				if (adjBus.getDirection() == L1Bus::Vertical && adjBus.side() == HMF::Coordinate::left) {
					candidates.push_back(*it);
				}
			} else {
				if (adjBus.getDirection() == L1Bus::Vertical && adjBus.side() == HMF::Coordinate::right) {
					candidates.push_back(*it);
				}
			}
		}
	}

	inline
	bool blocksUsedOutputBuffer(L1Bus const& bus) const
	{
		HRepeaterOnHICANN const& hrepeater = bus.toHLine().toHRepeaterOnHICANN();
		if (hrepeater.isSending())
		{
			HICANNGlobal const& hicann = bus.hicann();
			DNCMergerOnHICANN const& merger =
			    hrepeater.toSendingRepeaterOnHICANN().toDNCMergerOnHICANN();
			auto it = mOutbufMapping.find(hicann);
			if (it != mOutbufMapping.end() && !mOutbufMapping.at(hicann).empty(merger)) {
				return true;
			}
		}
		return false;
	}

	Route::BusSegment const mSource;
	std::unordered_set<HICANNGlobal>& mAllTargets;
	std::set<HICANNGlobal, SortByX> mTargets;
	std::unordered_map<size_t /*x*/, std::set<HICANNGlobal, SortByY>> mX;
	//std::unordered_map<size_t [>x*/, size_t /*other x<]> mOption;
	LastMile mLastMile;
	std::vector<routing_graph::vertex_descriptor>& mPredecessor;
	routing_graph const& mGraph;

	BusUsage const& mUsage;
	OutputMappingResult const& mOutbufMapping;
};

} // namespace

WaferRoutingBackbone::~WaferRoutingBackbone()
{}

Route::Segments
WaferRoutingBackbone::allocateRoute(
	Route::BusSegment const source,
	std::unordered_set<HICANNGlobal> targets,
	std::unordered_set<HICANNGlobal>& unreachable)
{
	auto const& routing_graph = getRoutingGraph();

	if (!unreachable.empty()) // we need an empty list
		throw std::runtime_error("unreachables must initially be empty");

	if (routing_graph[source].getDirection() != L1Bus::Horizontal) // we need an empty list
		throw std::runtime_error("source must be horizontal SPL1 repeater line");

	if (targets.empty()) // nothing todo
		return {};

	// TODO: a sparser hash map would be better here, because we are wasting a
	// lot of memory, num_vertices can be a rather large number.
	std::vector<int> distance(boost::num_vertices(routing_graph),
							  std::numeric_limits<int>::max());
	std::vector<routing_graph::vertex_descriptor> predecessor(boost::num_vertices(routing_graph));

	BackBoner boner(routing_graph, source, targets, predecessor, mUsage, *mOutbMapping);
	boner.find_path();
	std::unordered_map<HICANNGlobal, Route::BusSegment> lastMile =
		boner.last_mile();

	MAROCCO_INFO("finished backbone routing");

	// all targets not removed by visitor can be considered unreachable
	unreachable = targets;

	// collect all necesary bus segments representing this route by walking
	// backwards through the predecessor list.
	Route::Segments res;
	for (auto const target : lastMile)
	{
		std::vector<Route::BusSegment> _predecessor;
		Route::BusSegment cur = target.second;

		L1Bus const& targetBus =  routing_graph[cur];
		mUsage.increment(targetBus.hicann().toHICANNOnWafer(), targetBus);


		// insert all L1 segments into predecessor list
		// (for target to source, including the source segment)
		while (cur != source) {
			// remove all edges connecting this L1Bus vertex with any other
			// vertex, but leave it in the graph.
			clear_segment(cur);

			_predecessor.push_back(cur);
			//MAROCCO_INFO("cur: " << cur << " " << int(routing_graph[cur].getDirection() == L1Bus::Horizontal));
#if !defined(MAROCCO_NDEBUG)
			if (cur == predecessor.at(cur)) {
				throw std::runtime_error("cycle");
			}
			if (_predecessor.size() > 5000) {
				throw std::runtime_error("most certainly cycle");
			}
			cur = predecessor.at(cur);
#else
			cur = predecessor[cur];
#endif // MAROCCO_NDEBUG
		}
		// push source bus as last element
		_predecessor.push_back(source);

		// insert pre
		res[target.first] = std::move(_predecessor);
	}

	return res;
}

} // namespace routing
} // namespace marocco
