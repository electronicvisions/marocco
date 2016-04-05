#include <set>
#include <deque>
#include <vector>
#include <map>

#include "marocco/graph.h"
#include "marocco/placement/Result.h"
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

	// void insert(placement::OutputMappingResult const& output_mapping);

	template<typename Compare>
	void insert(placement::OutputMappingResult const& output_mapping,
	            std::set<HICANNGlobal, Compare> const& hicanns);

	bool empty() const;
	void pop();

	source_type source() const;
	std::vector<HardwareProjection> projections() const;

private:
	void insert(
		HICANNGlobal const& hicann,
		placement::OutputBufferMapping const& local_output_mapping);

	void insert(
		HICANNGlobal const& hicann,
		DNCMergerOnHICANN const& merger,
		std::vector<assignment::AddressMapping> const& sources);

	graph_t const& mGraph;
	pymarocco::PyMarocco const& mPyMarocco;

	std::map<source_type, std::vector<HardwareProjection>> mProjections;
	std::deque<std::pair<size_t, source_type>> mSources;
};


template<typename Compare>
void WaferRoutingPriorityQueue::insert(
	placement::OutputMappingResult const& output_mapping,
	std::set<HICANNGlobal, Compare> const& hicanns)
{
	for (auto const& hicann : hicanns)
	{
		auto const& local_output_mapping = output_mapping.at(hicann);
		insert(hicann, local_output_mapping);
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
