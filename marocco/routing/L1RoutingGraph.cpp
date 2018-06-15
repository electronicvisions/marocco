#include "marocco/routing/L1RoutingGraph.h"
#include "marocco/Logger.h"

#include "hal/Coordinate/iter_all.h"
#include "hal/HICANN/Crossbar.h"
#include "marocco/routing/PathBundle.h"

namespace marocco {
namespace routing {

using namespace HMF::Coordinate;

L1RoutingGraph::HICANN::HICANN(
    graph_type& graph,
    HICANNOnWafer const& hicann,
    parameters::L1Routing::SwitchOrdering switch_ordering,
    std::default_random_engine& random_engine)
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

	std::vector<std::pair<vertex_descriptor, vertex_descriptor>> switches;
	switches.reserve(
	    HMF::HICANN::Crossbar::periods * HMF::HICANN::Crossbar::period_length *
	    HLineOnHICANN::size);
	for (auto hline : iter_all<HLineOnHICANN>()) {
		for (auto vline : iter_all<VLineOnHICANN>()) {
			if (HMF::HICANN::Crossbar::exists(vline, hline)) {
				switches.push_back(std::make_pair(m_horizontal[hline], m_vertical[vline]));
			}
		}
	}

	switch (switch_ordering) {
		case parameters::L1Routing::SwitchOrdering::shuffle_switches_with_hicann_enum_as_seed:
			std::shuffle(switches.begin(), switches.end(), std::minstd_rand(hicann.id()));
			break;
		case parameters::L1Routing::SwitchOrdering::shuffle_switches_with_given_seed:
			std::shuffle(switches.begin(), switches.end(), random_engine);
			break;
		case parameters::L1Routing::SwitchOrdering::switches_in_filled_in_order:
			// nothing to do
			break;
		default:
			MAROCCO_ERROR("switch ordering " << static_cast<size_t>(switch_ordering) << " unknown");
			throw std::runtime_error("Unknown switch ordering");
	};

	for (auto const& sw : switches) {
		add_edge(sw.first, sw.second, graph);
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
		MAROCCO_ERROR(hicann << " not present in L1 routing graph");
		throw ResourceNotPresentError("HICANN not present in L1 routing graph");
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
		std::tie(std::ignore, success) = m_hicanns.insert(std::make_pair(
		    hicann, HICANN(
		                m_graph, hicann, m_l1_routing_parameters.switch_ordering(),
		                m_random_engine_shuffle_switches)));
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

void L1RoutingGraph::remove(PathBundle const& bundle)
{
	// To keep the vertex descriptors intact we only use clear_vertex() to remove all
	// edges connecting to a vertex instead of completely removing it from the graph.
	for (auto const& path : bundle.paths()) {
		for (vertex_descriptor const vertex : path) {
			clear_vertex(vertex, m_graph);
		}
	}
}

void L1RoutingGraph::remove(
    HMF::Coordinate::HICANNOnWafer const& hicann, HMF::Coordinate::HLineOnHICANN const& hline)
{
	// See note on clear_vertex above.
	clear_vertex(operator[](hicann)[hline], m_graph);
}

void L1RoutingGraph::remove(
    HMF::Coordinate::HICANNOnWafer const& hicann, HMF::Coordinate::VLineOnHICANN const& vline)
{
	// See note on clear_vertex above.
	clear_vertex(operator[](hicann)[vline], m_graph);
}

void L1RoutingGraph::remove(
    HMF::Coordinate::HICANNOnWafer const& hicann, HMF::Coordinate::HRepeaterOnHICANN const& hrep)
{
	auto hline = hrep.toHLineOnHICANN();
	auto side = hrep.toSideHorizontal();
	auto other_hicann = side == right ? hicann.east() : hicann.west();
	auto other_hline = side == right ? hline.east() : hline.west();

	auto vertex = operator[](hicann)[hline];

	auto it = m_hicanns.find(other_hicann);
	if (it == m_hicanns.end()) {
		return;
	}

	auto other_vertex = it->second[other_hline];
	remove_edge(vertex, other_vertex, m_graph);
}

void L1RoutingGraph::remove(
    HMF::Coordinate::HICANNOnWafer const& hicann, HMF::Coordinate::VRepeaterOnHICANN const& vrep)
{
	auto vline = vrep.toVLineOnHICANN();
	auto side = vrep.toSideVertical();
	auto other_hicann = side == top ? hicann.north() : hicann.south();
	auto other_vline = side == top ? vline.north() : vline.south();

	auto vertex = operator[](hicann)[vline];

	auto it = m_hicanns.find(other_hicann);
	if (it == m_hicanns.end()) {
		return;
	}

	auto other_vertex = it->second[other_vline];
	remove_edge(vertex, other_vertex, m_graph);
}

} // namespace routing
} // namespace marocco
