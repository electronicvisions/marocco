#include "marocco/routing/SynapseLossProxy.h"
#include <limits>

namespace marocco {
namespace routing {

SynapseLossProxy::value_type const SynapseLossProxy::NA =
	std::numeric_limits<SynapseLossProxy::value_type>::quiet_NaN();

#if !defined(MAROCCO_NO_SYNAPSE_TRACKING)
SynapseLossProxy::SynapseLossProxy(Matrix& weights, size_t& pre, size_t& post, size_t& set) :
	mWeights(weights), mChipPre(pre), mChipPost(post), mChipSet(set)
{}
#else
SynapseLossProxy::SynapseLossProxy(size_t& pre, size_t& post, size_t& set) :
	mChipPre(pre), mChipPost(post), mChipSet(set)
{}
#endif // MAROCCO_NO_SYNAPSE_TRACKING

void SynapseLossProxy::addLoss(size_t i1, size_t i2)
{
#if !defined(MAROCCO_NO_SYNAPSE_TRACKING)
#if !defined(MAROCCO_NDEBUG)
	if (i1>=mWeights.size1()) {
		throw std::runtime_error("out of range");
	}
	if (i2>=mWeights.size2()) {
		throw std::runtime_error("out of range");
	}
	if (!isRealWeight(mWeights(i1, i2))) {
		throw std::runtime_error("mask non-existant weight (proxy)");
	}
#endif // MAROCCO_NDEBUG

	mWeights(i1, i2) = NA;
#endif // MAROCCO_NO_SYNAPSE_TRACKING
	mChipPre++;
	mChipPost++;
}

void SynapseLossProxy::updateWeight(size_t i1, size_t i2, double value)
{
#if !defined(MAROCCO_NO_SYNAPSE_TRACKING)
#if !defined(MAROCCO_NDEBUG)
	if (i1>=mWeights.size1()) {
		throw std::runtime_error("out of range");
	}
	if (i2>=mWeights.size2()) {
		throw std::runtime_error("out of range");
	}
	if (!isRealWeight(mWeights(i1, i2))) {
		throw std::runtime_error("mask non-existant weight (proxy)");
	}
#endif // MAROCCO_NDEBUG

	mWeights(i1, i2) = value;
#endif // MAROCCO_NO_SYNAPSE_TRACKING
	mChipSet++;
}

void SynapseLossProxy::addRealized()
{
	mChipSet++;
}

void SynapseLossProxy::setWeight(size_t i1, size_t i2, double value)
{
#if !defined(MAROCCO_NO_SYNAPSE_TRACKING)
	updateWeight(i1,i2,value);
#endif // MAROCCO_NO_SYNAPSE_TRACKING

	addRealized();
}

} // namespace routing
} // namespace marocco
