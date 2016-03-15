#pragma once

#include <array>
#include <vector>
#include <set>
#include <unordered_map>
#include <boost/variant.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/breadth_first_search.hpp>

#include "hal/Coordinate/HMFGeometry.h"
#include "hal/Coordinate/iter_all.h"
#include "hal/Coordinate/typed_array.h"
#include "hal/HICANN/Merger.h"
#include "marocco/config.h"
#include "marocco/placement/NeuronBlockMapping.h"
#include "marocco/Logger.h"

namespace marocco {
namespace placement {

struct VertexType
{
	int level;
	int id;
};

struct EdgeType
{
	int port;
};

typedef boost::adjacency_list<
		boost::vecS,
		boost::vecS,
		boost::directedS,
		VertexType, // Vertex property
		EdgeType
	> merger_graph;

// FIXME: move this…
typedef boost::variant<
		HMF::Coordinate::Merger0OnHICANN,
		HMF::Coordinate::Merger1OnHICANN,
		HMF::Coordinate::Merger2OnHICANN,
		HMF::Coordinate::Merger3OnHICANN,
		HMF::Coordinate::DNCMergerOnHICANN
	> merger_coordinate;


class MergerTreeRouter
{
public:
	typedef merger_graph::vertex_descriptor Vertex;
	typedef merger_graph Graph;
	typedef chip_type<hardware_system_t>::type Chip;

	typedef std::unordered_map<
			HMF::Coordinate::NeuronBlockOnHICANN,
			HMF::Coordinate::DNCMergerOnHICANN
		> Result;

	MergerTreeRouter(HMF::Coordinate::HICANNGlobal const& hicann,
					 NeuronBlockMapping const& nbm,
					 Chip& chip,
					 resource_manager_t const& mgr);

	void run();

	HMF::Coordinate::HICANNGlobal const& hicann() const;

	Graph const& graph() const;

	Result const& result() const;

	void mergers_to_right_only();

private:
	size_t neurons(HMF::Coordinate::NeuronBlockOnHICANN const& nb) const;

	std::pair<std::set<HMF::Coordinate::NeuronBlockOnHICANN>, std::vector<int> >
	mergeable(HMF::Coordinate::NeuronBlockOnHICANN const& nb,
	    std::set<HMF::Coordinate::NeuronBlockOnHICANN> const& pending) const;

	static merger_coordinate coordinate(VertexType const& merger);
	void connect_in_tree(Vertex src, Vertex dest);

	/// number of placed neurons for each NeuronBlock
	HMF::Coordinate::typed_array<size_t, HMF::Coordinate::NeuronBlockOnHICANN> mNeurons;

	/// The actual boost graph representing the merge topology and the still
	/// available mergers during the process.
	Graph mMergerGraph;

	/// keep track of already configured mergers
	std::array<bool, 15+8> mTouched;

	/// the hicann we are currently trying to route. It's important, that the
	/// class is aware of its corresponding HICANN to be able to handle defects.
	HMF::Coordinate::HICANNGlobal mHICANN;

	/// Reference to hicann
	Chip& mChip;

	/// result from the MergerTree configuration
	Result mResult;
};

} // namespace placement
} // namespace marocco
