#include "marocco/routing/SynapseLoss.h"
#include "marocco/routing/SynapseLossImpl.h"

using namespace HMF::Coordinate;

namespace marocco {
namespace routing {

SynapseLoss::SynapseLoss(graph_t const& graph) :
	mImpl(new SynapseLossImpl(graph))
{}

void SynapseLoss::addLoss(Edge const& e,
						  Index const& src,
						  Index const& trg,
						  size_t i1,
						  size_t i2)
{
	mImpl->addLoss(e, src, trg, i1, i2);
}

void SynapseLoss::addLoss(Edge const& e,
						  Index const& src,
						  Index const& trg,
						  Assign const& bsrc,
						  Assign const& btrg)
{
	mImpl->addLoss(e, src, trg, bsrc, btrg);
}

SynapseLossProxy
SynapseLoss::getProxy(Edge const& e,
					  Index const& src,
					  Index const& trg)
{
	return mImpl->getProxy(e, src, trg);
}

void SynapseLoss::setWeight(Edge const& e,
					        Index const& trg,
								size_t i1,
								size_t i2,
								double value)
{
	mImpl->setWeight(e, trg, i1, i2, value);
}

void SynapseLoss::updateWeight(Edge const& e,
								size_t i1,
								size_t i2,
								double value)
{
	mImpl->updateWeight(e, i1, i2, value);
}

void SynapseLoss::addRealized( Index const& trg )
{
	mImpl->addRealized(trg);
}

SynapseLoss& SynapseLoss::operator+=(SynapseLoss const& rhs)
{
	*mImpl += *rhs.mImpl;
	return *this;
}

size_t SynapseLoss::getPreLoss(Index const& hicann) const
{
	return mImpl->getPreLoss(hicann);
}

size_t SynapseLoss::getPostLoss(Index const& hicann) const
{
	return mImpl->getPostLoss(hicann);
}

size_t SynapseLoss::getTotalLoss() const
{
	return mImpl->getTotalLoss();
}

size_t SynapseLoss::getTotalSynapses() const
{
	return mImpl->getTotalSynapses();
}

size_t SynapseLoss::getTotalSet() const
{
	return mImpl->getTotalSet();
}

SynapseLoss::Matrix const& SynapseLoss::getWeights(Edge const& e) const
{
	return static_cast<SynapseLossImpl const&>(*mImpl).getWeights(e);
}

void SynapseLoss::fill(pymarocco::MappingStats& stats) const
{
	mImpl->fill(stats);
}

} // namespace routing
} // namespace marocco
