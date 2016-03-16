#include <array>
#include <functional>
#include "marocco/placement/MergerTree.h"
#include "marocco/placement/OutputBufferMapping.h"
#include "marocco/Logger.h"

using namespace HMF::Coordinate;

namespace marocco {
namespace placement {

namespace {

struct UnroutableNeuronBlock {};

static std::array<size_t, 4> level_to_width = {{8, 4, 2, 1}};
static std::array<size_t, 4> level_to_offset = {{0, 8, 12, 14}};

MergerTreeRouter::Vertex vertex_for_dnc_merger(size_t ii) {
	if (ii >= 8) {
		throw std::runtime_error("Unexpected DNC merger index.");
	}
	return {15 + ii};
}

MergerTreeRouter::Vertex vertex_for_merger(size_t level, size_t ii) {
	if (ii >= level_to_width.at(level)) {
		throw std::runtime_error("Unexpected merger index for given level.");
	}
	return {level_to_offset.at(level) + ii};
}

} // namespace

MergerTreeRouter::MergerTreeRouter(HMF::Coordinate::HICANNGlobal const& hicann,
								   NeuronBlockMapping const& nbm,
								   Chip& chip,
								   resource_manager_t const& mgr) :
	mMergerGraph(15+8),
	mTouched(),
	mHICANN(hicann),
	mChip(chip)
{
	for (auto const& nb : iter_all<NeuronBlockOnHICANN>()) {
		mNeurons[nb] = nbm.neurons(nb);
	}

	for (auto& v : mTouched) {
		v = false;
	}

	// init dnc merger vertices
	for (size_t ii=0; ii<8; ++ii)
	{
		mMergerGraph[vertex_for_dnc_merger(ii)].level = 4;
		mMergerGraph[vertex_for_dnc_merger(ii)].id    = ii;
	}

	// init normal merger vertices
	size_t level=0, idx=0;
	while(level<4) {
		mMergerGraph[vertex_for_merger(level, idx)].level = level;
		mMergerGraph[vertex_for_merger(level, idx)].id    = idx;

		if (level>0) {
			add_edge(vertex_for_merger(level, idx), vertex_for_merger(level-1, 2*idx  ),
			         EdgeType{1}, mMergerGraph);
			add_edge(vertex_for_merger(level, idx), vertex_for_merger(level-1, 2*idx+1),
			         EdgeType{0}, mMergerGraph);
		}

		++idx;
		if (idx == level_to_width.at(level)) {
			++level;
			idx=0;
		}
	}

	// add edges from DNCMerger to normal mergers
	add_edge(vertex_for_dnc_merger(0), vertex_for_merger(0, 0),
	         EdgeType{0}, mMergerGraph);
	add_edge(vertex_for_dnc_merger(1), vertex_for_merger(1, 0),
	         EdgeType{0}, mMergerGraph);
	add_edge(vertex_for_dnc_merger(2), vertex_for_merger(0, 2),
	         EdgeType{0}, mMergerGraph);
	add_edge(vertex_for_dnc_merger(3), vertex_for_merger(3, 0),
	         EdgeType{0}, mMergerGraph);
	add_edge(vertex_for_dnc_merger(4), vertex_for_merger(0, 4),
	         EdgeType{0}, mMergerGraph);
	add_edge(vertex_for_dnc_merger(5), vertex_for_merger(2, 1),
	         EdgeType{0}, mMergerGraph);
	add_edge(vertex_for_dnc_merger(6), vertex_for_merger(1, 3),
	         EdgeType{0}, mMergerGraph);
	add_edge(vertex_for_dnc_merger(7), vertex_for_merger(0, 7),
	         EdgeType{0}, mMergerGraph);

	// remove defect mergers from the graph
#define HANDLE_DEFECTS(tier) \
	{ \
		auto const& res = defects->mergers ## tier (); \
		for (auto it=res->begin_disabled(); it!=res->end_disabled(); ++it) \
		{ \
			clear_vertex(vertex_for_merger(tier, it->value()), mMergerGraph); \
			MAROCCO_DEBUG("Marked " << *it << " on " << hicann << " as defect/disabled"); \
		} \
	}

	auto const& defects = mgr.get(hicann);

	MAROCCO_INFO("Disabling defect mergers");
	HANDLE_DEFECTS(0)
	HANDLE_DEFECTS(1)
	HANDLE_DEFECTS(2)
	HANDLE_DEFECTS(3)

	auto const& res = defects->dncmergers();
	for (auto it=res->begin_disabled(); it!=res->end_disabled(); ++it)
	{
		clear_vertex(vertex_for_dnc_merger(it->value()), mMergerGraph);
		MAROCCO_DEBUG("Marked " << *it << " on " << hicann << " as defect/disabled"); \
	}

#undef HANDLE_DEFECTS
}

void MergerTreeRouter::run()
{
	std::set<NeuronBlockOnHICANN> pending;
	for (auto const& nb : iter_all<NeuronBlockOnHICANN>())
	{
		if (neurons(nb) > 0) {
			pending.insert(nb);
		}
	}

	// Mergers are not all equally expensive. Those in the center and on
	// higher levels (closer to the DNCMergers) are more expensive. If one
	// wastes them, some neuron blocks in the center might become
	// unroutable. So we start the routing in the center and move outwards
	// and we collapse only adjacent neuron blocks.
	// Therefore it should always be possible to route all neuron blocks to
	// at least one SPL1 output at the expense of wasting L2 input bandwidth.

	static std::array<int, 9> const order = {{ -1, 5, 3, 1, 6, 4, 2, 7, 0 }};

	for (size_t jj=0; jj<order.size(); ++jj)
	{
		auto const ii = order[jj];
		NeuronBlockOnHICANN const nb(ii < 0 ? 3 : ii);

		// for jj>2 stuff might allready be merged into 3 or 5
		if (jj>2 && pending.find(nb) == pending.end()) {
			// nothing to do here
			continue;
		}

		std::set<NeuronBlockOnHICANN> merge;
		std::vector<int> predecessor;

		try { std::tie(merge, predecessor) = mergeable(nb, pending); }
		catch (UnroutableNeuronBlock const&)
		{
			warn(this) << "Unroutable mergers! This might be due to "
			              "defect mergers, you might want to blacklist"
			              "the corresponding neurons as well.";

			// TODO: only debuging, remove me later
			throw std::runtime_error("unroutable mergers");

			// there is nothing more to do here anymore, continue with other
			// mergers.
			continue;
		}

		// abort if not all could be merged at once
		if (ii < 0) {
			if (merge.size() < 8) {
				continue;
			}
		}

		DNCMergerOnHICANN const dnc_merger(nb);
		Vertex const dnc_merger_vertex = vertex_for_dnc_merger(dnc_merger);

		// establish the actual hardware merger configuration
		for (auto& adjacent_nb : merge)
		{
			Vertex cur = vertex_for_merger(0, adjacent_nb);
			while (cur != dnc_merger_vertex)
			{
				Vertex const next = predecessor.at(cur);
				connect_in_tree(cur, next);
				cur = next;
			}
		}

		Merger0OnHICANN const topmerger(nb.value());
		mChip.layer1[topmerger].config = HMF::HICANN::Merger::MERGE;

		// finaly, remove the allocated mergers from the merger tree and merged
		// NeuronBlocks from the pending list.
		for (auto& adjacent_nb : merge)
		{
			Vertex cur = vertex_for_merger(0, adjacent_nb);
			while (cur != dnc_merger_vertex) {
				clear_vertex(cur, mMergerGraph);
				cur = predecessor.at(cur);
			}

			// erase from pending
			pending.erase(adjacent_nb);

			// insert into results
			auto res = mResult.emplace(adjacent_nb, dnc_merger);
			if (!res.second) {
				throw std::runtime_error("NeuronBlock has already been inserted");
			}
		}

		if (pending.empty()) {
			break;
		}
	}
}

std::pair<std::set<NeuronBlockOnHICANN>, std::vector<int> >
MergerTreeRouter::mergeable(
    NeuronBlockOnHICANN const& main_nb,
    std::set<NeuronBlockOnHICANN> const& /*pending*/) const
{
	std::vector<size_t> distance(num_vertices(mMergerGraph), 0);
	std::vector<int> predecessor(num_vertices(mMergerGraph));

	// We try to merge as much adjacent neuron blocks as possible into
	// the DNC merger which would correspond to `main_nb` in a 1-to-1
	// configuration (directly “below” `main_nb`).
	Vertex const dnc_merger_vertex = vertex_for_dnc_merger(main_nb);

	boost::breadth_first_search(
		mMergerGraph, dnc_merger_vertex,
		boost::visitor(boost::make_bfs_visitor(std::make_pair(
			boost::record_distances(distance.data(), boost::on_tree_edge()),
			boost::record_predecessors(predecessor.data(),
			                           boost::on_tree_edge())))));

	// Make sure we can establish a MergerTree routing between the
	// main neuron block and `dnc_merger_vertex` at all.
	if (distance[vertex_for_merger(0, main_nb)] == 0) {
		throw UnroutableNeuronBlock{};
	}

	// Then select neuron blocks to merge, stopping on unreachables
	// candidates or when the maximum number of neurons is reached.
	// We do not allow merging of nonadjacent blocks to prevent
	// "muting" the block in between.

	size_t n = neurons(main_nb);
	std::set<NeuronBlockOnHICANN> mergeable{main_nb};

	auto merge = [distance, this, &mergeable](NeuronBlockOnHICANN nn) -> bool {
		if (distance[vertex_for_merger(0, nn)] > 0) {
			mergeable.insert(nn);
			return true;
		} else {
			return false;
		}
	};

	// to the LEFT
	for (size_t pos = main_nb.value(); pos--;) {
		NeuronBlockOnHICANN const l(pos);
		if (n + neurons(l) <= OutputBufferMapping::CAPACITY && merge(l)) {
			n += neurons(l);
		} else {
			break;
		}
	}

	// to the RIGHT
	for (size_t pos = main_nb.value() + 1; pos < NeuronBlockOnHICANN::end; ++pos) {
		NeuronBlockOnHICANN const r(pos);
		if (n + neurons(r) <= OutputBufferMapping::CAPACITY && merge(r)) {
			n += neurons(r);
		} else {
			break;
		}
	}

	return {mergeable, predecessor};
}

HICANNGlobal const& MergerTreeRouter::hicann() const
{
	return mHICANN;
}

MergerTreeRouter::Graph const& MergerTreeRouter::graph() const
{
	return mMergerGraph;
}

MergerTreeRouter::Result const& MergerTreeRouter::result() const
{
	return mResult;
}

size_t MergerTreeRouter::neurons(NeuronBlockOnHICANN const& nb) const
{
	return mNeurons[nb];
}

template<typename T>
void assign_merger(merger_coordinate& m, int ii)
{
	m = T{ii};
}

merger_coordinate MergerTreeRouter::coordinate(VertexType const& merger)
{
	typedef std::function<void(merger_coordinate&, int)> fun_t;
	static const std::array<fun_t, 5> lookup {{
		&assign_merger<Merger0OnHICANN>,
		&assign_merger<Merger1OnHICANN>,
		&assign_merger<Merger2OnHICANN>,
		&assign_merger<Merger3OnHICANN>,
		&assign_merger<DNCMergerOnHICANN>
	}};

	if (merger.level < 0 || merger.level > 4)
		throw std::runtime_error("whoops: level");

	merger_coordinate m;
	lookup[merger.level](m, merger.id);
	return m;
}

void MergerTreeRouter::connect_in_tree(Vertex const src, Vertex const dest)
{
	auto e = edge(dest, src, mMergerGraph);
	if (!e.second) {
		// FIXME: for debugging only
		throw std::runtime_error("mergers not connectable");
	}

	EdgeType edge = mMergerGraph[e.first];
	VertexType dd = mMergerGraph[dest];

	using namespace HMF::HICANN;

	auto& chip = mChip;
	auto& m = chip.layer1[coordinate(dd)];

	auto flag = edge.port == 1 ? Merger::LEFT_ONLY : Merger::RIGHT_ONLY;
	auto& t = mTouched.at(dest);

	if (m.config != flag && t) {
		m.config = Merger::MERGE;
	} else {
		m.config = flag;
	}

	t = true;
}

void MergerTreeRouter::mergers_to_right_only()
{
	auto& chip = mChip;
	merger_graph::vertex_iterator it, eit;
	std::tie(it, eit) = vertices(mMergerGraph);
	for (; it!=eit; ++it)
	{
		VertexType v = mMergerGraph[*it];
		auto& m = chip.layer1[coordinate(v)];
		m.config = HMF::HICANN::Merger::RIGHT_ONLY;
	}
}

} // namespace placement
} // namespace marocco
