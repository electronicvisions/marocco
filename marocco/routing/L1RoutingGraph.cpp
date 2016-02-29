#include "marocco/routing/L1RoutingGraph.h"

#include "hal/Coordinate/iter_all.h"
#include "hal/HICANN/Crossbar.h"

namespace marocco {
namespace routing {

using namespace HMF::Coordinate;

L1RoutingGraph::HICANN::HICANN(graph_type& graph, HICANNOnWafer const& hicann)
{
	for (auto hline : iter_all<HLineOnHICANN>()) {
		vertex_descriptor vertex = add_vertex(graph);
		m_horizontal[hline] = vertex;
		graph[vertex] = L1BusOnWafer(hicann, hline);
	}

	for (auto vline : iter_all<VLineOnHICANN>()) {
		vertex_descriptor vertex = add_vertex(graph);
		m_vertical[vline] = vertex;
		graph[vertex] = L1BusOnWafer(hicann, vline);
	}

	for (auto hline : iter_all<HLineOnHICANN>()) {
		for (auto vline : iter_all<VLineOnHICANN>()) {
			if (HMF::HICANN::Crossbar::exists(vline, hline)) {
				add_edge(m_horizontal[hline], m_vertical[vline], graph);
			}
		}
	}
}

auto L1RoutingGraph::HICANN::operator[](HLineOnHICANN const& hline) const -> vertex_descriptor
{
	return m_horizontal[hline];
}

auto L1RoutingGraph::HICANN::operator[](VLineOnHICANN const& vline) const -> vertex_descriptor
{
	return m_vertical[vline];
}

auto L1RoutingGraph::graph() -> graph_type&
{
	return m_graph;
}

auto L1RoutingGraph::graph() const -> graph_type const&
{
	return m_graph;
}

auto L1RoutingGraph::operator[](vertex_descriptor vertex) -> value_type&
{
	return m_graph[vertex];
}

auto L1RoutingGraph::operator[](vertex_descriptor vertex) const -> value_type const&
{
	return m_graph[vertex];
}

auto L1RoutingGraph::operator[](HICANNOnWafer const& hicann) const -> HICANN const&
{
	auto it = m_hicanns.find(hicann);
	if (it == m_hicanns.end()) {
		throw ResourceNotPresentError("HICANN not present in graph");
	}
	return it->second;
}

auto L1RoutingGraph::operator[](value_type bus) const -> vertex_descriptor
{
	auto const& hicann = operator[](bus.toHICANNOnWafer());
	if (bus.is_horizontal()) {
		return hicann[bus.toHLineOnHICANN()];
	} else {
		return hicann[bus.toVLineOnHICANN()];
	}
}

template <typename LineT>
bool L1RoutingGraph::connect(
	HICANNOnWafer const& hicann,
	HICANNOnWafer (HICANNOnWafer::*conv)() const,
	LineT (LineT::*line_conv)() const)
{
	try {
		HICANN const& current = m_hicanns.at(hicann);
		HICANN const& other = m_hicanns.at((hicann.*conv)());

		for (auto line : iter_all<LineT>()) {
			auto other_line = (line.*line_conv)();
			add_edge(current[line], other[other_line], m_graph);
		}

		return true;
	} catch (std::out_of_range const&) {
		// HICANN not present in L1RoutingGraph
	} catch (std::overflow_error const&) {
		// reached bound of wafer, other HICANN does not exist
	} catch (std::domain_error const&) {
		// invalid combination of X and Y (can happen because wafer is round)
	}

	return false;
}

void L1RoutingGraph::add(HICANNOnWafer const& hicann)
{
	{
		bool success;
		std::tie(std::ignore, success) =
		    m_hicanns.insert(std::make_pair(hicann, HICANN(m_graph, hicann)));
		if (!success) {
			throw std::runtime_error("HICANN already present in graph");
		}
	}

	// Try to connect this HICANN to surrounding HICANNs.
	connect(hicann, &HICANNOnWafer::north, &VLineOnHICANN::north);
	connect(hicann, &HICANNOnWafer::east, &HLineOnHICANN::east);
	connect(hicann, &HICANNOnWafer::south, &VLineOnHICANN::south);
	connect(hicann, &HICANNOnWafer::west, &HLineOnHICANN::west);
}

} // namespace routing
} // namespace marocco
