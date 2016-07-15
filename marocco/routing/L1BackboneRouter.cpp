#include "marocco/routing/L1BackboneRouter.h"

#include "marocco/Logger.h"

using namespace HMF::Coordinate;

namespace marocco {
namespace routing {

L1BackboneRouter::L1BackboneRouter(
    L1GraphWalker const& walker,
    vertex_descriptor const& source,
    score_function_type const& vertical_scoring)
	: m_walker(walker),
	  m_graph(walker.graph()),
	  m_source(source),
	  m_vertical_scoring(vertical_scoring)
{
	if (!m_graph[m_source].is_horizontal()) {
		throw std::invalid_argument("source has to be horizontal bus");
	}
}

void L1BackboneRouter::add_target(target_type const& target)
{
	m_targets[target.x()][target.y()] = target;
}

PathBundle::path_type L1BackboneRouter::path_to(target_type const& target) const
{
	auto it = m_vertex_for_targets.find(target);
	if (m_predecessors.empty() || it == m_vertex_for_targets.end()) {
		return {};
	}
	return path_from_predecessors(m_predecessors, it->second);
}

auto L1BackboneRouter::source() const -> vertex_descriptor
{
	return m_source;
}

void L1BackboneRouter::run()
{
	m_predecessors = std::vector<vertex_descriptor>(boost::num_vertices(m_graph));
	m_predecessors[m_source] = m_source;

	// Walk horizontally until we reach the leftmost/rightmost HICANN.
	for (auto const direction : {east, west}) {
		// All targets may have already been removed in the first iteration.
		if (m_targets.empty()) {
			return;
		}

		size_t const limit = direction == west ? m_targets.begin()->first.value()
		                                       : m_targets.rbegin()->first.value();
		L1GraphWalker::path_type path;
		bool reached_limit;
		std::tie(path, reached_limit) = m_walker.walk(m_source, direction, limit);

		// Try a detour by walking a short segment in vertical direction.
		while (!reached_limit) {
			L1GraphWalker::path_type detour;
			vertex_descriptor detour_start = path.empty() ? m_source : path.back();
			MAROCCO_DEBUG(
			    "Could not reach " << direction << "ernmost HICANN.\nTrying to detour from "
			                       << m_graph[detour_start]);
			std::tie(detour, reached_limit) =
			    m_walker.detour_and_walk(detour_start, direction, limit);
			// L1GraphWalker::detour_and_walk guarantees that the detour advances
			// horizontally by at least one HICANN, if it is not empty.
			if (detour.empty()) {
				MAROCCO_DEBUG("Detour unsuccessful");
				break;
			}
			std::copy(detour.begin(), detour.end(), std::back_inserter(path));
		}

		// Store predecessors and vertices for targets.
		vertex_descriptor predecessor = m_source;
		for (auto const& vertex : path) {
			m_predecessors[vertex] = predecessor;
			predecessor = vertex;

			auto hicann = m_graph[vertex].toHICANNOnWafer();
			if (m_graph[vertex].is_vertical() && is_target(hicann)) {
				m_vertex_for_targets[hicann] = vertex;
			}

			maybe_branch_off_to_vertical_targets(vertex);
		}
	}

	maybe_branch_off_to_vertical_targets(m_source);
}

void L1BackboneRouter::maybe_branch_off_to_vertical_targets(vertex_descriptor const& vertex)
{
	if (!m_graph[vertex].is_horizontal()) {
		return;
	}

	auto it = m_targets.find(m_graph[vertex].toHICANNOnWafer().x());
	if (it == m_targets.end()) {
		return;
	}

	auto const& y_targets = it->second;
	// `add_target()` guarantees that there is at least one entry for this value of x().
	assert(!y_targets.empty());

	bool const scoring_function_provided = !!m_vertical_scoring;

	auto candidates = m_walker.change_orientation(vertex);
	if (candidates.empty()) {
		MAROCCO_WARN("No candidates for vertical branch in backbone routing");
		return;
	}

	// To establish a connection to vertical targets we consider all connected vertical
	// buses and choose the best one based on a score.

	size_t best_score = 0;
	vertex_descriptor best_candidate;
	for (auto const& candidate : candidates) {
		size_t score = 1;
		// Walk vertically until we reach the topmost/bottommost HICANN.
		for (auto const direction : {north, south}) {
			size_t const limit = direction == north ? y_targets.begin()->first.value()
			                                        : y_targets.rbegin()->first.value();
			// `candidate` is included in path to account for targets on the current HICANN.
			L1GraphWalker::path_type path{candidate};
			{
				L1GraphWalker::path_type tail;
				std::tie(tail, std::ignore) = m_walker.walk(candidate, direction, limit);
				std::copy(tail.begin(), tail.end(), std::back_inserter(path));
			}

			// Increase score for each reached target.
			for (vertex_descriptor const other : path) {
				auto const hicann = m_graph[other].toHICANNOnWafer();
				if (y_targets.find(hicann.y()) == y_targets.end()) {
					continue;
				}
				score += scoring_function_provided ? m_vertical_scoring(other) : 1;
			}
		}

		if (score > best_score) {
			best_candidate = candidate;
			best_score = score;
		}
	}

	for (auto const direction : {north, south}) {
		size_t const limit = direction == north ? y_targets.begin()->first.value()
		                                        : y_targets.rbegin()->first.value();
		L1GraphWalker::path_type path{best_candidate};
		{
			L1GraphWalker::path_type tail;
			std::tie(tail, std::ignore) = m_walker.walk(best_candidate, direction, limit);
			std::copy(tail.begin(), tail.end(), std::back_inserter(path));
		}

		// Store predecessors and vertices for targets.
		vertex_descriptor predecessor = vertex;
		for (vertex_descriptor const other : path) {
			m_predecessors[other] = predecessor;
			predecessor = other;

			auto const hicann = m_graph[other].toHICANNOnWafer();
			if (m_graph[other].is_vertical() && y_targets.find(hicann.y()) != y_targets.end()) {
				m_vertex_for_targets[hicann] = other;
			}
		}
	}

	// Remove targets with given X coordinate, even if they were not reached, because we
	// only want to walk each column once.
	m_targets.erase(it);
}

bool L1BackboneRouter::is_target(target_type const& hicann) const
{
	auto it = m_targets.find(hicann.x());
	if (it != m_targets.end()) {
		auto const& map = it->second;
		auto it_ = map.find(hicann.y());
		if (it_ != map.end()) {
			return true;
		}
	}
	return false;
}

} // namespace routing
} // namespace marocco
