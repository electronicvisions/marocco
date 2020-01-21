#pragma once

#include <boost/shared_ptr.hpp>
#include "marocco/graph.h"
#include "marocco/config.h"
#include "marocco/assignment/PopulationSlice.h"
#include "marocco/routing/SynapseLossProxy.h"

namespace pymarocco {
class MappingStats;
}
namespace marocco {
namespace routing {

class SynapseLossImpl;

class SynapseLoss
{
public:
	typedef euter::Connector::matrix_type Matrix;
	typedef graph_t::edge_descriptor Edge;
	typedef graph_t::vertex_descriptor Vertex;
	typedef halco::hicann::v2::HICANNOnWafer Index;
	typedef assignment::PopulationSlice Assign;

	SynapseLoss(graph_t const& graph);

	void addLoss(Edge const& e,
				 Index const& src,
				 Index const& trg,
				 size_t i1,
				 size_t i2);

	void addLoss(Edge const& e,
				 Index const& src,
				 Index const& trg,
				 Assign const& bsrc,
				 Assign const& btrg);

	SynapseLossProxy getProxy(Edge const& e,
							  Index const& src,
							  Index const& trg);

	void setWeight(Edge const& e, Index const& trg, size_t i1, size_t i2, double w);

	void addRealized(Index const& trg);

#if !defined(MAROCCO_NO_SYNAPSE_TRACKING)
	// set the distorted weight value
	void updateWeight(Edge const& e, size_t i1, size_t i2, double w);
#endif // MAROCCO_NO_SYNAPSE_TRACKING

	/// merge two SynapseLossImpl instances
	SynapseLoss& operator+=(SynapseLoss const& rhs);

	size_t getPreLoss(Index const& hicann) const;
	size_t getPostLoss(Index const& hicann) const;
	size_t getTotalLoss() const;
	size_t getTotalSynapses() const;
	size_t getTotalSet() const;

	Matrix const& getWeights(Edge const& e) const;

	void fill(pymarocco::MappingStats& stats) const;

private:
	boost::shared_ptr<SynapseLossImpl> mImpl;
};

} // namespace routing
} // namespace marocco
