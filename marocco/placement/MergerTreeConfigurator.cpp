#include "marocco/placement/MergerTreeConfigurator.h"

#include <functional>
#include <boost/graph/breadth_first_search.hpp>
#include <boost/variant.hpp>

#include "hal/Coordinate/iter_all.h"

using namespace HMF::Coordinate;

namespace marocco {
namespace placement {

namespace {

typedef boost::
    variant<Merger0OnHICANN, Merger1OnHICANN, Merger2OnHICANN, Merger3OnHICANN, DNCMergerOnHICANN>
        merger_variant_type;

merger_variant_type coordinate(MergerTreeGraph::vertex_type const& merger)
{
	switch (merger.level) {
		case 0:
			return Merger0OnHICANN(merger.index);
		case 1:
			return Merger1OnHICANN(merger.index);
		case 2:
			return Merger2OnHICANN(merger.index);
		case 3:
			return Merger3OnHICANN(merger.index);
		case 4:
			return DNCMergerOnHICANN(merger.index);
	}
	throw std::logic_error("unexpected merger level");
}

} // namespace

MergerTreeConfigurator::MergerTreeConfigurator(
	sthal::Layer1& layer1,
	MergerTreeGraph const& graph)
	: m_layer1(layer1), m_graph(graph)
{
	std::fill(m_configured.begin(), m_configured.end(), false);
}

void MergerTreeConfigurator::run(MergerRoutingResult::mapped_type const& mapping)
{
	DNCMergerOnHICANN last_merger(0);
	for (auto const& dnc : mapping) {
		if (dnc < last_merger) {
			throw std::runtime_error("DNC mergers should be monotonically increasing");
		}
		last_merger = dnc;
	}

	for (auto const nb : iter_all<NeuronBlockOnHICANN>()) {
		connect(nb, mapping[nb]);
	}
}

void MergerTreeConfigurator::run(MergerTreeRouter::result_type const& mapping)
{
	DNCMergerOnHICANN last_merger(0);
	for (auto const& item : mapping) {
		if (item.second < last_merger) {
			throw std::runtime_error("DNC mergers should be monotonically increasing");
		}
		last_merger = item.second;
	}
	for (auto const& item : mapping) {
		connect(item.first, item.second);
	}
}

void MergerTreeConfigurator::connect(NeuronBlockOnHICANN const& nb, DNCMergerOnHICANN const& dnc_merger)
{
	auto const& graph = m_graph.graph();
	Merger0OnHICANN const top_merger(nb.value());
	auto const source_merger_vertex = m_graph[top_merger];
	auto const dnc_merger_vertex = m_graph[dnc_merger];

	std::vector<size_t> distance(num_vertices(graph), 0u);
	std::vector<MergerTreeGraph::vertex_descriptor> predecessors(num_vertices(graph));

	boost::breadth_first_search(
	    graph, dnc_merger_vertex,
	    boost::visitor(
	        boost::make_bfs_visitor(
	            std::make_pair(
	                boost::record_distances(distance.data(), boost::on_tree_edge()),
	                boost::record_predecessors(predecessors.data(), boost::on_tree_edge())))));

	if (distance[source_merger_vertex] == 0u) {
		throw std::runtime_error("invalid merger configuration");
	}

	for (auto cur = source_merger_vertex; cur != dnc_merger_vertex; cur = predecessors.at(cur)) {
		auto const next = predecessors.at(cur);
		connect(cur, next);
	}

	// Merge events from the background generators.
	m_layer1[top_merger].config = HMF::HICANN::Merger::MERGE;
}

void MergerTreeConfigurator::connect(
    MergerTreeGraph::vertex_descriptor src, MergerTreeGraph::vertex_descriptor dest)
{
	auto const& graph = m_graph.graph();
	// Edges are directed “upwards”, so `dest` has to be passed in before `src`.
	auto const e = edge(dest, src, graph);
	assert(e.second);

	auto const port = graph[e.first];
	auto const dest_merger = graph[dest];
	auto const flag =
	    (port == left) ? HMF::HICANN::Merger::LEFT_ONLY : HMF::HICANN::Merger::RIGHT_ONLY;
	auto& merger_cfg = m_layer1[coordinate(dest_merger)].config;
	auto& configured = m_configured[dest];

	// We do not touch mergers that are already configured to merge their inputs.
	// This is needed to keep the default configuration (MERGE) for DNC mergers,
	// see #1369 for why this is desirable.
	if (merger_cfg == HMF::HICANN::Merger::MERGE) {
		return;
	}

	if ((merger_cfg != flag) && configured) {
		merger_cfg = HMF::HICANN::Merger::MERGE;
	} else {
		merger_cfg = flag;
	}

	configured = true;
}

} // namespace placement
} // namespace marocco
