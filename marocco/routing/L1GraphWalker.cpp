#include "marocco/routing/L1GraphWalker.h"

#include "hal/Coordinate/geometry.h"
#include "hal/Coordinate/iter_all.h"
#include "marocco/util/iterable.h"

using namespace HMF::Coordinate;

namespace marocco {
namespace routing {

L1GraphWalker::L1GraphWalker(
    graph_type const& graph, boost::optional<resource::HICANNManager> resource_manager) :
    m_graph(graph),
    m_res_mgr(resource_manager)
{}


void L1GraphWalker::avoid_using(vertex_descriptor const& vertex)
{
	m_avoid.insert(vertex);
}

auto L1GraphWalker::change_orientation(vertex_descriptor const& vertex) const
    -> std::vector<vertex_descriptor>
{
	std::vector<vertex_descriptor> result;
	auto orientation = m_graph[vertex].toOrientation();
	for (auto other : make_iterable(boost::adjacent_vertices(vertex, m_graph))) {
		if (m_graph[other].toOrientation() == orientation) {
			continue;
		}

		if (m_avoid.find(other) != m_avoid.end()) {
			continue;
		}

		result.push_back(other);
	}
	return result;
}

bool L1GraphWalker::step(vertex_descriptor& vertex, Direction const& direction) const
{
	if (m_graph[vertex].toOrientation() != direction.toOrientation()) {
		throw std::invalid_argument(
			"orientation of vertex does not allow specified direction");
	}

	HICANNOnWafer destination;
	try {
		destination = m_graph[vertex].toHICANNOnWafer().move(direction);
	} catch (std::overflow_error const&) {
		// reached bound of wafer, other HICANN does not exist
		return false;
	} catch (std::domain_error const&) {
		// invalid combination of X and Y (can happen because wafer is round)
		return false;
	}

	for (auto other : make_iterable(boost::adjacent_vertices(vertex, m_graph))) {
		if (m_graph[other].toHICANNOnWafer() != destination) {
			continue;
		}

		if (m_avoid.find(other) != m_avoid.end()) {
			continue;
		}

		vertex = other;
		return true;
	}
	return false;
}

#define WALK_IMPL(DIRECTION, CONDITION)                                                            \
	path_type path;                                                                                \
	vertex_descriptor current = vertex;                                                            \
	while ((m_graph[current].toHICANNOnWafer().CONDITION) && step(current, DIRECTION)) {           \
		path.push_back(current);                                                                   \
	}                                                                                              \
	return std::make_pair(path, !(m_graph[current].toHICANNOnWafer().CONDITION));

auto L1GraphWalker::walk_north(
    vertex_descriptor const& vertex, HICANNOnWafer::y_type const& limit) const
	-> std::pair<path_type, bool>
{
	WALK_IMPL(north, y() > limit);
}

auto L1GraphWalker::walk_east(
    vertex_descriptor const& vertex, HICANNOnWafer::x_type const& limit) const
	-> std::pair<path_type, bool>
{
	WALK_IMPL(east, x() < limit);
}

auto L1GraphWalker::walk_south(
    vertex_descriptor const& vertex, HICANNOnWafer::y_type const& limit) const
	-> std::pair<path_type, bool>
{
	WALK_IMPL(south, y() < limit);
}

auto L1GraphWalker::walk_west(
    vertex_descriptor const& vertex, HICANNOnWafer::x_type const& limit) const
	-> std::pair<path_type, bool>
{
	WALK_IMPL(west, x() > limit);
}

#undef WALK_IMPL

auto L1GraphWalker::walk(
    vertex_descriptor const& vertex, Direction const& direction, size_t limit) const
	-> std::pair<path_type, bool>
{
	if (direction == east) {
		return walk_east(vertex, X(limit));
	}
	if (direction == south) {
		return walk_south(vertex, Y(limit));
	}
	if (direction == west) {
		return walk_west(vertex, X(limit));
	}
	return walk_north(vertex, Y(limit));
}

auto L1GraphWalker::detour_and_walk(
    vertex_descriptor const& vertex, Direction const& direction, size_t limit) const
    -> std::pair<path_type, bool>
{
	return detour_and_walk(vertex, direction, limit, path_type(boost::num_vertices(m_graph)));
}

auto L1GraphWalker::detour_and_walk(
    vertex_descriptor const& vertex,
    Direction const& direction,
    size_t limit,
    path_type predecessors) const -> std::pair<path_type, bool>
{
	if (m_graph[vertex].toOrientation() != direction.toOrientation()) {
		throw std::invalid_argument(
			"orientation of vertex does not allow specified direction");
	}

	// Check if the source vertex is already beyond the specified limit.
	{
		auto const hicann = m_graph[vertex].toHICANNOnWafer();
		if ((direction == north && hicann.y() <= limit) ||
		    (direction == east && hicann.x() >= limit) ||
		    (direction == south && hicann.y() >= limit) ||
		    (direction == west && hicann.x() <= limit)) {
			return std::make_pair(path_type{}, true);
		}
	}

	size_t longest = 0;
	path_type best_detour;

	auto candidates = change_orientation(vertex);
	candidates = L1_crossbar_restrictioning(vertex, candidates, predecessors, m_res_mgr, m_graph);

	// Iterate over all L1 buses with different orientation.
	for (auto const& candidate : candidates) {
		for (auto const perpendicular : iter_all<Direction>()) {
			// Consider both perpendicular directions.
			if (perpendicular.toOrientation() == direction.toOrientation()) {
				continue;
			}

			// Step to other HICANNs in the perpendicular direction.
			// On each HICANN try to walk in the original direction again, as far as
			// possible.  If the limit is reached, the algorithm returns immediately.
			// Else the search is continued for all possible HICANNs and the detour with
			// the longest extension in the original direction is chosen.
			path_type detour{candidate};
			vertex_descriptor other = candidate;
			while (step(other, perpendicular)) {
				detour.push_back(other);
				auto candidates_ = change_orientation(other);
				candidates_ = L1_crossbar_restrictioning(
				    other, candidates_, predecessors, m_res_mgr, m_graph);
				for (auto const& candidate_ : candidates_) {
					path_type extension;
					bool reached_limit;
					std::tie(extension, reached_limit) = walk(candidate_, direction, limit);
					// As `longest` starts at zero, we only accept detours that have a
					// positive extension along the original direction.
					if (extension.size() > longest) {
						best_detour = detour;
						best_detour.push_back(candidate_);
						std::copy(
						    extension.begin(), extension.end(), std::back_inserter(best_detour));
						longest = extension.size();
						if (reached_limit) {
							return std::make_pair(best_detour, true);
						}
					}
				}
			}
		}
	}

	return std::make_pair(best_detour, false);
}

auto L1GraphWalker::graph() const -> graph_type const&
{
	return m_graph;
}

} // namespace routing
} // namespace marocco
