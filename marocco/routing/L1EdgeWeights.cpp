#include "marocco/routing/L1EdgeWeights.h"

namespace marocco {
namespace routing {

L1EdgeWeights::L1EdgeWeights(graph_type const& graph)
	: m_graph(graph)
{
}

void L1EdgeWeights::set_weight(edge_descriptor const& edge, weight_type weight)
{
	if (weight < 1) {
		throw std::invalid_argument("weight has to be non-zero");
	}
	m_edge_weights[edge] = weight;
}

void L1EdgeWeights::set_weight(vertex_descriptor const& vertex, weight_type weight)
{
	if (weight < 1) {
		throw std::invalid_argument("weight has to be non-zero");
	}
	m_vertex_weights[vertex] = weight;
}

auto L1EdgeWeights::weight(edge_descriptor const& edge) const -> weight_type
{
	weight_type weight = 1;

	auto it = m_edge_weights.find(edge);
	if (it != m_edge_weights.end()) {
		return it->second;
	}

	// Note that edges in the routing graph are undirected.
	for (auto const vertex : {boost::source(edge, m_graph), boost::target(edge, m_graph)}) {
		auto it = m_vertex_weights.find(vertex);
		if (it != m_vertex_weights.end()) {
			weight = std::max(weight, it->second);
		}
	}

	return weight;
}

auto L1EdgeWeights::graph() const -> graph_type const&
{
	return m_graph;
}

} // namespace routing
} // namespace marocco
