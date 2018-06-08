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
	typedef boost::property_map<graph_t, projection_t>::const_type proj_map_t;
	typedef HMF::Coordinate::HICANNGlobal HICANNGlobal;
	typedef HMF::Coordinate::OutputBufferOnHICANN OutputBufferOnHICANN;
	typedef std::pair<HICANNGlobal, OutputBufferOnHICANN> source_t;

public:
	WaferRoutingPriorityQueue(
		graph_t const& graph,
		pymarocco::PyMarocco const& pymarocco,
		proj_map_t projmap);

	void insert(placement::OutputMappingResult const& output_mapping);

	template<typename Compare>
	void insert(placement::OutputMappingResult const& output_mapping,
	            std::set<HICANNGlobal, Compare> const& hicanns);

	bool empty() const;
	void pop();

	source_t source() const;
	std::vector<HardwareProjection> projections() const;

private:
	void insert(
		HICANNGlobal const& hicann,
		placement::OutputBufferMapping const& local_output_mapping);

	void insert(
		HICANNGlobal const& hicann,
		OutputBufferOnHICANN const& ob,
		std::vector<assignment::AddressMapping> const& sources);

	graph_t const& mGraph;
	pymarocco::PyMarocco const& mPyMarocco;
	proj_map_t mProjectionViews;

	std::map<source_t, std::vector<HardwareProjection>> mProjections;
	std::deque<std::pair<size_t, source_t>> mSources;
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

	using p = std::pair<size_t, source_t>;
	std::stable_sort(mSources.begin(), mSources.end(),
		[](p const & a, p const & b) -> bool
		{
			return a.first < b.first;
		});
}

} // namespace routing
} // namespace marocco
