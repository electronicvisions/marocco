#include "marocco/placement/MergerTreeRouter.h"

#include <boost/graph/breadth_first_search.hpp>

#include <array>

#include "hal/Coordinate/iter_all.h"
#include "marocco/Logger.h"
#include "marocco/placement/internal/L1AddressPool.h"

using namespace HMF::Coordinate;

namespace marocco {
namespace placement {

namespace {

struct UnroutableNeuronBlock {};

} // namespace

MergerTreeRouter::MergerTreeRouter(
	MergerTreeGraph const& graph, internal::Result::denmem_assignment_type::mapped_type const& nbm)
	: m_graph(graph)
{
	// Count the number of mapped bio neurons for each neuron block.
	for (auto const& nb : iter_all<NeuronBlockOnHICANN>()) {
		m_neurons[nb] = 0u;
		for (std::shared_ptr<internal::NeuronPlacementRequest> const& pl : nbm[nb]) {
			m_neurons[nb] += pl->population_slice().size();
		}
	}
}

void MergerTreeRouter::run()
{
	std::set<NeuronBlockOnHICANN> pending;
	for (auto const& nb : iter_all<NeuronBlockOnHICANN>()) {
		if (m_neurons[nb] > 0) {
			pending.insert(nb);
		}
	}

	if (pending.empty()) {
		return;
	}

	// Mergers are not all equally expensive. Those in the center and on
	// higher levels (closer to the DNCMergers) are more expensive. If one
	// wastes them, some neuron blocks in the center might become
	// unroutable. So we start the routing in the center and move outwards
	// and we collapse only adjacent neuron blocks.
	// Therefore it should always be possible to route all neuron blocks to
	// at least one SPL1 output at the expense of wasting L2 input bandwidth.

	static std::array<int, 9> const order = {{ -1, 5, 3, 1, 6, 4, 2, 7, 0 }};

	for (size_t jj = 0; jj < order.size(); ++jj) {
		auto const ii = order[jj];

		// We try to route as many neuron blocks adjacent to `nb` as possible into
		// `dnc_merger`.  For the special case `ii = -1` we try to merge all blocks
		// into DNCMergerOnHICANN(3).
		DNCMergerOnHICANN const dnc_merger(ii < 0 ? 3 : ii);
		NeuronBlockOnHICANN const nb(dnc_merger);

		// For jj > 2 the neuron block might already be merged into DNC mergers 3 or 5.
		if (jj > 2 && pending.find(nb) == pending.end()) {
			continue;
		}

		std::set<NeuronBlockOnHICANN> adjacent_nbs;
		std::vector<MergerTreeGraph::vertex_descriptor> predecessors;

		try {
			std::tie(adjacent_nbs, predecessors) = mergeable(dnc_merger);
		} catch (UnroutableNeuronBlock const&) {
			MAROCCO_WARN(
			    "Failed to route all neuron blocks. This might be due to defect mergers. "
			    "Consider blacklisting the corresponding neurons as well.");

			// TODO: only debuging, remove me later
			throw std::runtime_error("unroutable mergers");

			// there is nothing more to do here anymore, continue with other
			// mergers.
			continue;
		}

		// Only allow special case if all neuron blocks can be routed to a single DNC merger.
		if (ii < 0) {
			if (adjacent_nbs.size() < NeuronBlockOnHICANN::size) {
				continue;
			}
		}

		// As #mergeable() returns as many adjacent neuron blocks as possible, an additional check
		// is necessary that the leftmost/rightmost neuron block actually contains any neurons.
		// If this is not the case, the corresponding column remains available for external input
		// (which needs a one-to-one connection to the background generators).
		// Note that we always keep NeuronBlockOnHICANN(3), since this column contains mergers
		// relevant for connecting neuron blocks from both the left and right side of the HICANN.

		// Delete leading neuron blocks with zero placed neurons.
		for (auto it = adjacent_nbs.begin(); it != adjacent_nbs.end();
		     it = adjacent_nbs.erase(it)) {
			if (m_neurons[*it] > 0 || *it > NeuronBlockOnHICANN(2)) {
				break;
			}
			MAROCCO_TRACE("removing " << *it << " from the left");
		}

		// Delete trailing neuron blocks with zero placed neurons.
		for (auto it = adjacent_nbs.rbegin(); it != adjacent_nbs.rend();
		     it = decltype(it){adjacent_nbs.erase(std::next(it).base())}) {
			if (m_neurons[*it] > 0 || *it < NeuronBlockOnHICANN(4)) {
				break;
			}
			MAROCCO_TRACE("removing " << *it << " from the right");
		}

		auto& graph = m_graph.graph();
		auto dnc_merger_vertex = m_graph[dnc_merger];
		for (auto& adjacent_nb : adjacent_nbs) {
			// Remove used mergers from the graph.
			auto cur = m_graph[Merger0OnHICANN(nb)];
			while (cur != dnc_merger_vertex) {
				clear_vertex(cur, graph);
				cur = predecessors.at(cur);
			}

			pending.erase(adjacent_nb);

			auto res = m_result.insert(std::make_pair(adjacent_nb, dnc_merger));
			assert(res.second);
		}

		if (pending.empty()) {
			return;
		}
	}
}

std::pair<std::set<HMF::Coordinate::NeuronBlockOnHICANN>,
          std::vector<MergerTreeGraph::vertex_descriptor> >
MergerTreeRouter::mergeable(
    DNCMergerOnHICANN const& merger)
{
	auto const& graph = m_graph.graph();

	std::vector<size_t> distance(num_vertices(graph), 0u);
	std::vector<MergerTreeGraph::vertex_descriptor> predecessors(num_vertices(graph));

	// We try to merge as many adjacent neuron blocks as possible into the specified DNC merger.
	auto const dnc_merger_vertex = m_graph[merger];

	boost::breadth_first_search(
	    graph, dnc_merger_vertex,
	    boost::visitor(
	        boost::make_bfs_visitor(
	            std::make_pair(
	                boost::record_distances(distance.data(), boost::on_tree_edge()),
	                boost::record_predecessors(predecessors.data(), boost::on_tree_edge())))));

	// Make sure we can establish a MergerTree routing between the specified merger and
	// the corresponding neuron block at all.  This main neuron block is the one which
	// would be naturally connected to this DNC merger in a 1-to-1 configuration.
	NeuronBlockOnHICANN main_nb(merger);
	if (distance[m_graph[Merger0OnHICANN(main_nb)]] == 0u) {
		throw UnroutableNeuronBlock{};
	}

	// Then select neuron blocks to merge, stopping on unreachables
	// candidates or when the maximum number of neurons is reached.
	// We do not allow merging of nonadjacent blocks to prevent
	// "muting" the block in between.

	size_t bio_neurons_count = m_neurons[main_nb];
	std::set<NeuronBlockOnHICANN> mergeable{main_nb};

	auto merge = [distance, this, &mergeable](NeuronBlockOnHICANN nb) -> bool {
		if (distance[m_graph[Merger0OnHICANN(nb)]] > 0u) {
			mergeable.insert(nb);
			return true;
		} else {
			return false;
		}
	};

	// to the LEFT
	for (int pos = main_nb.value() - 1; pos >= 0; --pos) {
		NeuronBlockOnHICANN const nb{NeuronBlockOnHICANN::value_type(pos)};
		if (bio_neurons_count + m_neurons[nb] <= internal::L1AddressPool::capacity() && merge(nb)) {
			bio_neurons_count += m_neurons[nb];
		} else {
			break;
		}
	}

	// to the RIGHT
	for (size_t pos = main_nb.value() + 1; pos < NeuronBlockOnHICANN::end; ++pos) {
		NeuronBlockOnHICANN const nb(pos);
		if (bio_neurons_count + m_neurons[nb] <= internal::L1AddressPool::capacity() && merge(nb)) {
			bio_neurons_count += m_neurons[nb];
		} else {
			break;
		}
	}

	return {mergeable, predecessors};
}

auto MergerTreeRouter::result() const -> result_type const&
{
	return m_result;
}

} // namespace placement
} // namespace marocco
