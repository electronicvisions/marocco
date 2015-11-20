#include <algorithm>

#include "marocco/routing/WaferRoutingPriorityQueue.h"

#include "hal/Coordinate/iter_all.h"
#include "marocco/util/iterable.h"
#include "marocco/routing/HardwareProjection.h"

using namespace HMF::Coordinate;

namespace marocco {
namespace routing {

WaferRoutingPriorityQueue::WaferRoutingPriorityQueue(
	graph_t const& graph,
	pymarocco::PyMarocco const& pymarocco)
	: mGraph(graph),
	  mPyMarocco(pymarocco),
	  mProjections(),
	  mSources() {}

bool WaferRoutingPriorityQueue::empty() const
{
	return mSources.empty();
}

WaferRoutingPriorityQueue::source_t
WaferRoutingPriorityQueue::source() const
{
	if (mSources.empty())
		throw std::runtime_error("No more items.");

	return mSources.back().second;
}

std::vector<HardwareProjection>
WaferRoutingPriorityQueue::projections() const
{
	return mProjections.at(source());
}

void WaferRoutingPriorityQueue::pop()
{
	auto const src = source();
	mProjections.erase(src);

	while (!mSources.empty() &&
	       mProjections.find(mSources.back().second) == mProjections.end())
	{
		mSources.pop_back();
	}
}

void WaferRoutingPriorityQueue::insert(
	placement::OutputMappingResult const& output_mapping)
{
	for (auto const& om : output_mapping)
	{
		auto const& hicann = om.first;
		auto const& local_output_mapping = om.second;
		insert(hicann, local_output_mapping);
	}

	using p = std::pair<size_t, source_t>;
	std::stable_sort(mSources.begin(), mSources.end(),
		[](p const & a, p const & b) -> bool
		{
			return a.first < b.first;
		});
}

void WaferRoutingPriorityQueue::insert(
	HICANNGlobal const& hicann,
	placement::OutputBufferMapping const& local_output_mapping)
{
	for (auto const& ob : iter_all<OutputBufferOnHICANN>())
	{
		std::vector<assignment::AddressMapping> const& sources =
			local_output_mapping.at(ob);

		insert(hicann, ob, sources);
	}
}

void WaferRoutingPriorityQueue::insert(
	HICANNGlobal const& hicann,
	OutputBufferOnHICANN const& ob,
	std::vector<assignment::AddressMapping> const& sources)
{
	for (auto const& source : sources)
	{
		// skip if the source population has no outgoing connections
		if (out_degree(source.bio().population(), mGraph) == 0) {
			info(this) << "terminal population without outgoing connections";
			continue;
		}

		for (auto const& edge : make_iterable(out_edges(source.bio().population(), mGraph))) {
			HardwareProjection hw_proj{source, edge};

			ProjectionView const proj_view = mGraph[edge];
			size_t proj_id = proj_view.projection()->id();

			size_t priority = mPyMarocco.routing_priority.get(proj_id);
			auto source = std::make_pair(hicann, ob);
			mProjections[source].push_back(std::move(hw_proj));
			mSources.emplace_front(std::move(priority), std::move(source));
		}
	}
}

} // namespace routing
} // namespace marocco
