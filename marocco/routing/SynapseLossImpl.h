#pragma once

#include <boost/serialization/concurrent_unordered_map.h>
#include <tbb/concurrent_unordered_map.h>
#include <oneapi/tbb/mutex.h>

#include "halco/hicann/v2/hicann.h"

#include "marocco/assignment/PopulationSlice.h"
#include "marocco/graph.h"
#include "marocco/routing/SynapseLossProxy.h"

namespace pymarocco {
class MappingStats;
}

namespace marocco {
namespace routing {

class SynapseLossImpl
{
public:
	typedef SynapseLossProxy::Matrix Matrix;
	typedef SynapseLossProxy::value_type value_Type;

	typedef graph_t::edge_descriptor Edge;
	typedef halco::hicann::v2::HICANNOnWafer Index;
	typedef assignment::PopulationSlice Assign;

	SynapseLossImpl(graph_t const& graph);

	// TODO: handle synapse loss of external inputs as well.

	/// tag single weights as lost
	void addLoss(Edge const& e,
				 Index const& src,
				 Index const& trg,
				 size_t i1,
				 size_t i2);

	/// tag weight ranges as lost
	void addLoss(Edge const& e,
				 Index const& src,
				 Index const& trg,
				 Assign const& bsrc,
				 Assign const& btrg);

	/// set synapse as realized in target
	void addRealized(Index const& trg);

	/// set synapse as realized in target and update weight value with the distorted one
	/// calls addRealized(..) and updateWeight(..)
	void setWeight(Edge const& e,
				   Index const& trg,
				   size_t i1,
				   size_t i2,
				   double value);

#if !defined(MAROCCO_NO_SYNAPSE_TRACKING)
	/// update weight, i.e. set the distorted weight value
	void updateWeight(Edge const& e,
				   size_t i1,
				   size_t i2,
				   double value);
#endif // MAROCCO_NO_SYNAPSE_TRACKING

	/// return proxy object for loss handling, to speedup loss handling
	SynapseLossProxy getProxy(Edge const& e,
							  Index const& src,
							  Index const& trg);

	/// merge two SynapseLossImpl instances
	SynapseLossImpl& operator+=(SynapseLossImpl const& rhs);

	size_t getPreLoss(Index const& hicann) const;
	size_t getPostLoss(Index const& hicann) const;
	size_t getTotalLoss() const;
	size_t getTotalSynapses() const;
	size_t getTotalSet() const;

	/// fill weight matrices of MappingStats
	void fill(pymarocco::MappingStats& stats) const;

	static inline bool isRealWeight(double w)
	{
		return !std::isnan(w) && w > 0.;
	}

	Matrix const& getWeights(Edge const& e) const;

private:
#if !defined(MAROCCO_NO_SYNAPSE_TRACKING)
	Matrix& getWeights(Edge const& e);

	/// tracks synapse changes on a per ProjectionView basis.
	tbb::concurrent_unordered_map<Edge, Matrix, std::hash<Edge> > mWeights;
#endif // MAROCCO_NO_SYNAPSE_TRACKING

	/// tracks number synapse of lost synapses on a HICANN basis.
	tbb::concurrent_unordered_map<Index, size_t, std::hash<Index> > mChipPre;
	tbb::concurrent_unordered_map<Index, size_t, std::hash<Index> > mChipPost;

	tbb::concurrent_unordered_map<Index, size_t, std::hash<Index> > mChipSet;

	graph_t const& mGraph;

	tbb::mutex mMutex;
};

} // namespace routing
} // namespace marocco
