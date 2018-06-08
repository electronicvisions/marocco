#pragma once

#include "marocco/graph.h"

namespace marocco {
namespace routing {

class SynapseLossProxy
{
public:
	typedef Connector::matrix_type Matrix;
	typedef Matrix::value_type value_type;

	static value_type const NA;

#if !defined(MAROCCO_NO_SYNAPSE_TRACKING)
	SynapseLossProxy(Matrix& weights, size_t& pre, size_t& post, size_t& set);
#else
	SynapseLossProxy(size_t& pre, size_t& post, size_t& set);
#endif // MAROCCO_NO_SYNAPSE_TRACKING

	void addLoss(size_t i1, size_t i2);
	void setWeight(size_t i1, size_t i2, double value);
	void addRealized();
#if !defined(MAROCCO_NO_SYNAPSE_TRACKING)
	void updateWeight(size_t i1, size_t i2, double value);
#endif // MAROCCO_NO_SYNAPSE_TRACKING

	static inline bool isRealWeight(value_type w)
	{
		return !std::isnan(w) && w > 0.;
	}

private:
#if !defined(MAROCCO_NO_SYNAPSE_TRACKING)
	Matrix& mWeights;
#endif // MAROCCO_NO_SYNAPSE_TRACKING
	size_t& mChipPre;
	size_t& mChipPost;
	size_t& mChipSet;
};

} // namespace routing
} // namespace marocco
