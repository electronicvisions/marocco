#include "marocco/routing/WaferRoutingPriorityQueue.h"

#include <algorithm>

#include "hal/Coordinate/iter_all.h"
#include "marocco/Logger.h"
#include "marocco/routing/HardwareProjection.h"
#include "marocco/util/iterable.h"

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

WaferRoutingPriorityQueue::source_type
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
	placement::NeuronPlacementResult const& neuron_placement, HICANNGlobal const& hicann)
{
	for (auto const& merger : iter_all<DNCMergerOnHICANN>()) {
		insert(neuron_placement, hicann, merger);
	}
}

void WaferRoutingPriorityQueue::insert(
	placement::NeuronPlacementResult const& neuron_placement,
	HICANNGlobal const& hicann,
	DNCMergerOnHICANN const& merger)
{
	for (auto const& item : neuron_placement.find(DNCMergerOnWafer(merger, hicann))) {
		// skip if the source population has no outgoing connections
		if (out_degree(item.population(), mGraph) == 0) {
			MAROCCO_INFO("terminal population without outgoing connections");
			continue;
		}

		auto const& address = item.address();
		assert(address != boost::none);
		assignment::AddressMapping address_mapping(
		    assignment::PopulationSlice(item.population(), item.neuron_index(), 1),
		    {address->toL1Address()});

		for (auto const& edge : make_iterable(out_edges(item.population(), mGraph))) {
			HardwareProjection hw_proj{address_mapping, edge};

			ProjectionView const proj_view = mGraph[edge];
			size_t proj_id = proj_view.projection()->id();

			size_t priority = mPyMarocco.routing_priority.get(proj_id);
			auto source = std::make_pair(hicann, merger);
			mProjections[source].push_back(std::move(hw_proj));
			mSources.emplace_front(std::move(priority), std::move(source));
		}
	}
}

} // namespace routing
} // namespace marocco
