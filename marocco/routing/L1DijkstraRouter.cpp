#include "marocco/routing/L1DijkstraRouter.h"

#include <algorithm>
#include <vector>
#include <boost/graph/dijkstra_shortest_paths.hpp>
#include <boost/property_map/property_map.hpp>

#include "marocco/util/event_visitor.h"
#include "marocco/util/function_property_map.h"
#include "marocco/util/to_function.h"

namespace marocco {
namespace routing {

L1DijkstraRouter::L1DijkstraRouter(
    L1EdgeWeights const& weights,
    vertex_descriptor const& source,
    SwitchExclusiveness exclusiveness)
    : m_weights(weights)
    , m_graph(weights.graph())
    , m_source(source)
    , m_exclusiveness(exclusiveness)
    , m_targets()
{}

void L1DijkstraRouter::add_target(target_type const& target)
{
	m_targets.insert(std::make_pair(target, target_vertices_type()));
}

void L1DijkstraRouter::run()
{
	if (m_targets.empty()) {
		return;
	}

	using namespace std::placeholders;

	// num_vertices() is 122 880 for a wafer with all HICANNs.
	// sizeof(vertex_descriptor) =~ 4 byte ⇒ ~0.5 MiB (not much!)
	m_predecessors = std::vector<vertex_descriptor>(boost::num_vertices(m_graph));
	std::iota(m_predecessors.begin(), m_predecessors.end(), 0);

	auto visitor = boost::make_dijkstra_visitor(
		make_event_visitor(
			to_function(&L1DijkstraRouter::finish_vertex, this, _1, _2),
			boost::on_finish_vertex()));
	auto weight_map =
		make_function_property_map(to_function(&L1EdgeWeights::weight, &m_weights, _1));

	boost::dijkstra_shortest_paths(
		m_graph, m_source,
		boost::weight_map(weight_map)
		.predecessor_map(
			boost::make_iterator_property_map(
				m_predecessors.begin(), get(boost::vertex_index, m_graph)))
		.visitor(visitor));
}

auto L1DijkstraRouter::vertices_for(target_type const& target) const -> target_vertices_type const&
{
	auto it = m_targets.find(target);
	if (it == m_targets.end()) {
		throw std::runtime_error("trying to get vertices for non-registered target");
	}
	return it->second;
}

PathBundle::path_type L1DijkstraRouter::path_to(vertex_descriptor const& target) const
{
	if (m_predecessors.empty()) {
		return {};
	}
	return path_from_predecessors(m_predecessors, target);
}

void L1DijkstraRouter::finish_vertex(vertex_descriptor const& vertex, graph_type const& graph)
{
	auto const& bus = graph[vertex];
	auto target = Target(bus.toHICANNOnWafer(), bus.toOrientation());
	auto it = m_targets.find(target);
	if (it == m_targets.end()) {
		return;
	}

	// We have to make sure that this target does not violate the constraint of using only
	// one crossbar switch per vertical or horizontal line on each HICANN.  We do this by
	// keeping track of switches used in paths to registered targets.  Targets that would
	// use a switch that is already in use are discarded.  Note that this prefers targets
	// nearer to the source because of the traversal order in Dijkstra's algorithm.
	// In the current hardware revision lines are swapped in such a way that for
	// horizontal lines there is only one bus for each target HICANN that fulfills this
	// constraint.

	switch (m_exclusiveness) {
		case SwitchExclusiveness::per_route:
			// Switch constraints are only considered on a per-target / per-route basis.
			m_used_switches.clear();
			break;
		case SwitchExclusiveness::global:
			break;
		default:
			throw std::runtime_error("unknown switch exclusiveness");
	}

	std::vector<vertex_descriptor> rollback;
	auto current = vertex;
	while (true) {
		auto previous = m_predecessors[current];

		auto const current_bus_is_vertical = m_graph[current].is_vertical();
		auto const previous_bus_is_vertical = m_graph[previous].is_vertical();

		if (current_bus_is_vertical ^ previous_bus_is_vertical) {
			auto const vertical = current_bus_is_vertical ? current : previous;
			auto const horizontal = current_bus_is_vertical ? previous : current;
			auto ret_h = m_used_switches.insert(std::make_pair(horizontal, vertical));
			auto ret_v = m_used_switches.insert(std::make_pair(vertical, horizontal));
			if (ret_h.second && ret_v.second) {
				rollback.push_back(horizontal);
				rollback.push_back(vertical);
			} else {
				// Switch is already in use ⇒ rollback changes and discard target.

				switch (m_exclusiveness) {
					case SwitchExclusiveness::global:
						for (auto const& vtx : rollback) {
							m_used_switches.erase(vtx);
						}
						return;
					case SwitchExclusiveness::per_route:
						// not necessary to rollback changes
						return;
					default:
						throw std::runtime_error("unknown switch exclusiveness");
				}
			}
		}

		if (previous == current) {
			break;
		}

		current = previous;
	}

	it->second.insert(vertex);
}

auto L1DijkstraRouter::source() const -> vertex_descriptor
{
	return m_source;
}

} // namespace routing
} // namespace marocco
