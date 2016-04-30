#include <set>
#include <deque>
#include <vector>
#include <map>

#include "marocco/graph.h"
#include "marocco/placement/results/Placement.h"
#include "pymarocco/PyMarocco.h"

namespace marocco {
namespace routing {

class HardwareProjection;

class WaferRoutingPriorityQueue
{
	typedef HMF::Coordinate::HICANNGlobal HICANNGlobal;
	typedef HMF::Coordinate::DNCMergerOnHICANN DNCMergerOnHICANN;
	typedef std::pair<HICANNGlobal, DNCMergerOnHICANN> source_type;

public:
	WaferRoutingPriorityQueue(
		graph_t const& graph,
		pymarocco::PyMarocco const& pymarocco);

	template <typename Compare>
	void insert(
		placement::results::Placement const& neuron_placement,
		std::set<HICANNGlobal, Compare> const& hicanns);

	bool empty() const;
	void pop();

	source_type source() const;
	std::vector<HardwareProjection> projections() const;

private:
	void insert(
		placement::results::Placement const& neuron_placement,
		HICANNGlobal const& hicann);

	void insert(
		placement::results::Placement const& neuron_placement,
		HICANNGlobal const& hicann,
		DNCMergerOnHICANN const& merger);

	graph_t const& mGraph;
	pymarocco::PyMarocco const& mPyMarocco;

	std::map<source_type, std::vector<HardwareProjection>> mProjections;
	std::deque<std::pair<size_t, source_type>> mSources;
};


template<typename Compare>
void WaferRoutingPriorityQueue::insert(
	placement::results::Placement const& neuron_placement,
	std::set<HICANNGlobal, Compare> const& hicanns)
{
	for (auto const& hicann : hicanns) {
		insert(neuron_placement, hicann);
	}

	using p = std::pair<size_t, source_type>;
	std::stable_sort(mSources.begin(), mSources.end(),
		[](p const & a, p const & b) -> bool
		{
			return a.first < b.first;
		});
}

} // namespace routing
} // namespace marocco
