#include "marocco/routing/SynapseLossImpl.h"
#include "marocco/routing/util.h"
#include "marocco/Logger.h"
#include "pymarocco/MappingStats.h"

using namespace HMF::Coordinate;

namespace marocco {
namespace routing {

SynapseLossImpl::SynapseLossImpl(graph_t const& graph) :
	mGraph(graph)
{}

void SynapseLossImpl::addLoss(Edge const& e,
							  Index const& src,
							  Index const& trg,
							  size_t i1,
							  size_t i2)
{
#ifndef MAROCCO_NO_SYNAPSE_TRACKING
	// first mask away original weight
	auto& weights = getWeights(e);

#ifndef MAROCCO_NDEBUG
	// do we really want to check whether (i1,i2) references a finite weight > 0.
	ProjectionView const view = mGraph[e];
	auto const w = view.getWeights()(i1, i2);
	if (!SynapseLossProxy::isRealWeight(w)) {
		throw std::runtime_error("add loss for non-existant weight");
	}

	if (!SynapseLossProxy::isRealWeight(weights(i1, i2))) {
		throw std::runtime_error("mask non-existant weight");
	}
#endif // MAROCCO_NDEBUG

	weights(i1, i2) = SynapseLossProxy::NA;
#endif // MAROCCO_NO_SYNAPSE_TRACKING

	// then insert source loss
	mChipPre[src] += 1;

	// and finally insert target loss
	mChipPost[trg] += 1;
}

void SynapseLossImpl::addLoss(Edge const& e,
							  Index const& src,
							  Index const& trg,
							  Assign const& bsrc,
							  Assign const& btrg)
{
#ifndef MAROCCO_NO_SYNAPSE_TRACKING
	// first mask away original weight
	auto& weights = getWeights(e);
#endif // MAROCCO_NO_SYNAPSE_TRACKING

	ProjectionView const view = mGraph[e];

	// calculate offsets for pre and post populations in this view
	size_t const src_neuron_offset_in_proj_view =
		to_relative_index(view.pre().mask(), bsrc.offset());
	size_t const trg_neuron_offset_in_proj_view =
		to_relative_index(view.post().mask(), btrg.offset());

	auto const& _ws = view.getWeights();

	size_t cnt = 0;
	size_t src_neuron_in_proj_view = src_neuron_offset_in_proj_view;
	for (size_t src_neuron=bsrc.offset(); src_neuron<bsrc.size()+bsrc.offset(); ++src_neuron)
	{
		if (!view.pre().mask()[src_neuron]) {
			continue;
		}

		size_t trg_neuron_in_proj_view = trg_neuron_offset_in_proj_view;
		for (size_t trg_neuron=btrg.offset(); trg_neuron<btrg.size()+btrg.offset(); ++trg_neuron)
		{
			// first check whether neurons are part of ProjectionView
			if (!view.post().mask()[trg_neuron]) {
				continue;
			}

			// then check whether this would be an existent weight
			auto const w = _ws(src_neuron_in_proj_view, trg_neuron_in_proj_view);
			if (SynapseLossProxy::isRealWeight(w))
			{
#ifndef MAROCCO_NO_SYNAPSE_TRACKING
				weights(src_neuron_in_proj_view, trg_neuron_in_proj_view) = SynapseLossProxy::NA;
#endif // MAROCCO_NO_SYNAPSE_TRACKING
				cnt++;
			}
			trg_neuron_in_proj_view++;
		}
		src_neuron_in_proj_view++;
	}

	// then insert source loss
	mChipPre[src] += cnt;

	// and finally insert target loss
	mChipPost[trg] += cnt;
}

void SynapseLossImpl::setWeight(Edge const& e,
								Index const& trg,
								size_t i1,
								size_t i2,
								double value)
{
#ifndef MAROCCO_NO_SYNAPSE_TRACKING
	updateWeight(e,i1,i2,value);
#endif // MAROCCO_NO_SYNAPSE_TRACKING

	addRealized(trg);
}

void SynapseLossImpl::addRealized( Index const& trg)
{
	// add a realized synapse at the target
	mChipSet[trg] += 1;
}

#ifndef MAROCCO_NO_SYNAPSE_TRACKING
void SynapseLossImpl::updateWeight(Edge const& e,
								size_t i1,
								size_t i2,
								double value)
{
	// first mask away original weight
	auto& weights = getWeights(e);

#ifndef MAROCCO_NDEBUG
	// do we really want to check whether (i1,i2) references a finite weight > 0.
	ProjectionView const view = mGraph[e];
	auto const w = view.getWeights()(i1, i2);
	if (!SynapseLossProxy::isRealWeight(w)) {
		throw std::runtime_error("add loss for non-existant weight");
	}

	if (!SynapseLossProxy::isRealWeight(weights(i1, i2))) {
		throw std::runtime_error("mask non-existant weight");
	}
#endif // MAROCCO_NDEBUG

	weights(i1, i2) = value;
}
#endif // MAROCCO_NO_SYNAPSE_TRACKING


SynapseLossProxy
SynapseLossImpl::getProxy(Edge const& e,
						  Index const& src,
						  Index const& trg)
{
#ifndef MAROCCO_NO_SYNAPSE_TRACKING
	auto& weights = getWeights(e);
	return SynapseLossProxy(weights, mChipPre[src], mChipPost[trg], mChipSet[trg]);
#else
	return SynapseLossProxy(mChipPre[src], mChipPost[trg], mChipSet[trg]);
#endif // MAROCCO_NO_SYNAPSE_TRACKING
}

/// merge two SynapseLossImpl instances
SynapseLossImpl& SynapseLossImpl::operator+=(SynapseLossImpl const& rhs)
{
	if (&mGraph != &rhs.mGraph) {
		throw std::runtime_error("unmergeable instances");
	}

#ifndef MAROCCO_NO_SYNAPSE_TRACKING
	for (auto const& entry : rhs.mWeights)
	{
		auto it = mWeights.find(entry.first);
		if (it != mWeights.end()) {
			// we need to merge
			ProjectionView const view = mGraph[entry.first];

			auto const& orig = view.getWeights();
			auto const& src  = entry.second;
			auto& trg = it->second;
			for (size_t i1=0; i1<orig.size1(); ++i1)
			{
				for (size_t i2=0; i2<orig.size2(); ++i2)
				{
					if (src(i1, i2) != orig(i1, i2))
					{
						if (trg(i1, i2) != orig(i1, i2)) {
							throw std::runtime_error("synapese modified more than once");
						}
						trg(i1, i2) = src(i1, i2);
					}
				}
			}
		} else {
			mWeights[entry.first] = entry.second;
		}
	}
#endif // MAROCCO_NO_SYNAPSE_TRACKING

	for (auto const& entry : rhs.mChipPost)
	{
		auto it = mChipPost.find(entry.first);
		if (it != mChipPost.end()) {
			throw std::runtime_error("one HICANN handled twice");
		}
		mChipPost[entry.first] = entry.second;
	}

	for (auto const& entry : rhs.mChipPre)
	{
		mChipPre[entry.first] += entry.second;
	}

	return *this;
}

size_t SynapseLossImpl::getPreLoss(Index const& hicann) const
{
	return mChipPre.at(hicann);
}

size_t SynapseLossImpl::getPostLoss(Index const& hicann) const
{
	return mChipPost.at(hicann);
}

size_t SynapseLossImpl::getTotalLoss() const
{
	size_t cnt = 0;
	for (auto const& entry : mChipPost)
	{
		cnt += entry.second;
	}
	return cnt;
}

size_t SynapseLossImpl::getTotalSynapses() const
{
	size_t cnt = 0;
	graph_t::edge_iterator it, eit;
	std::tie(it, eit) = boost::edges(mGraph);
	for (; it!=eit; ++it)
	{
		ProjectionView const proj = mGraph[*it];

		auto const& weights = proj.getWeights();
		for (size_t i1=0; i1<weights.size1(); ++i1)
		{
			for (size_t i2=0; i2<weights.size2(); ++i2)
			{
				if (SynapseLossProxy::isRealWeight(weights(i1, i2))) {
					cnt++;
				}
			}
		}
	}
	return cnt;
}

size_t SynapseLossImpl::getTotalSet() const
{
	size_t cnt=0;
	for (auto const& entry : mChipSet) {
		cnt += entry.second;
	}
	return cnt;
}

void SynapseLossImpl::fill(pymarocco::MappingStats& stats) const
{
	graph_t::edge_iterator it, eit;

#ifndef MAROCCO_NO_SYNAPSE_TRACKING
	// first prepare weight matrices
	std::tie(it, eit) = boost::edges(mGraph);
	for (; it!=eit; ++it)
	{
		ProjectionView const proj_view = mGraph[*it];
		Projection const& proj = *proj_view.projection();

		auto& weights = stats.getWeights(proj.id());
		if (weights.size1()==0) {
			debug(this) << "copying weight matrix";
			weights = proj.getWeights().get();
		}

		try {
			auto const& sl_weights = getWeights(*it);

			size_t pre_cnt = 0;
			for (PopulationView const& view : proj.pre()) {
				if (view == proj_view.pre()) {
					break;
				} else {
					pre_cnt += view.size();
				}
			}

			size_t post_cnt = 0;
			for (PopulationView const& view : proj.post()) {
				if (view == proj_view.post()) {
					break;
				} else {
					post_cnt += view.size();
				}
			}

			// now we have the offsets
			for (size_t i1 = 0; i1 < proj_view.pre().size(); ++i1) {
				for (size_t i2 = 0; i2 < proj_view.post().size(); ++i2) {
					weights(pre_cnt + i1, post_cnt + i2) = sl_weights(i1, i2);
				}
			}
		} catch (std::out_of_range const&) {
			// no synapse loss for this combination
		}
	}
#endif // MAROCCO_NO_SYNAPSE_TRACKING

	// finally fill in the numbers
	stats.setSynapseLoss(getTotalLoss());
	stats.setSynapses(getTotalSynapses());
	stats.setSynapsesSet(getTotalSet());
}

SynapseLossImpl::Matrix const& SynapseLossImpl::getWeights(Edge const& e) const
{
#ifndef MAROCCO_NO_SYNAPSE_TRACKING
	return mWeights.at(e);
#else
	return Matrix{};
#endif // MAROCCO_NO_SYNAPSE_TRACKING
}

#ifndef MAROCCO_NO_SYNAPSE_TRACKING
SynapseLossImpl::Matrix& SynapseLossImpl::getWeights(Edge const& e)
{
	auto it = mWeights.find(e);
	if (it==mWeights.end()) {
		ProjectionView const view = mGraph[e];
		mMutex.lock();
		auto res = mWeights.insert(std::make_pair(e, Matrix(view.getWeights())));
		mMutex.unlock();
		if (!res.second) {
			/// during concurrent insert it can happen, that one is faster than
			/// the other, leading to insuccessful inserts, eventough the
			/// element is there.
			auto iit = mWeights.find(e);
			if (iit==mWeights.end()) {
				throw std::runtime_error("unable to allocate weights");
			} else {
				return iit->second;
			}
		}
		return res.first->second;
	}
	return it->second;
}
#endif // MAROCCO_NO_SYNAPSE_TRACKING

} // namespace routing
} // namespace marocco
