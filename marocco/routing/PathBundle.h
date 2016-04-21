#pragma once

#include <functional>
#include <vector>
#include <boost/iterator/transform_iterator.hpp>

#include "marocco/routing/L1RoutingGraph.h"
#include "marocco/util/iterable.h"

namespace marocco {
namespace routing {

/**
 * @brief Collection of paths with common starting point in the routing graph.
 * @invariant All contained paths are non-empty and have a common source.
 */
class PathBundle
{
public:
	typedef L1RoutingGraph::graph_type graph_type;
	typedef graph_type::vertex_descriptor vertex_descriptor;
	typedef std::vector<vertex_descriptor> path_type;
	typedef std::vector<path_type> bundle_type;
	typedef std::function<vertex_descriptor(path_type const&)> vertex_from_bundle_type;
	typedef boost::transform_iterator<vertex_from_bundle_type, bundle_type::const_iterator>
		vertex_iterator;

	PathBundle();

	/**
	 * @brief Constructs a new bundle containing one path.
	 * @throw std::invalid_argument if \c path is empty.
	 */
	PathBundle(path_type const& path);

	/**
	 * @brief Constructs a new bundle from the given paths.
	 * @throw std::invalid_argument if any path is empty or if there are multiple sources.
	 */
	PathBundle(bundle_type const& paths);

	/**
	 * @brief Adds a path to this bundle.
	 * @throw std::invalid_argument if \c path is empty.
	 */
	void add(path_type const& path);

	/**
	 * @brief Returns the common source of the contained paths.
	 */
	vertex_descriptor source() const;

	/**
	 * @brief Returns iterators over the targets of the contained paths.
	 */
	iterable<vertex_iterator> targets() const;

	/**
	 * @brief Returns the contained paths.
	 */
	bundle_type const& paths() const;

	/**
	 * @brief Retuns the number of contained paths.
	 */
	size_t size() const;

	/**
	 * @brief Checks whether this bundle does not contain any paths.
	 */
	bool empty() const;

private:
	std::vector<path_type> m_paths;
}; // PathBundle

/**
 * @brief Extracts a sequence of vertices from a predecessor map.
 * Given a target vertex and a predecessor map, extract the path to the target vector.
 * This is implemented by iteratively looking up the preceeding vertex until the current
 * and preceeding vertex are the same (stopping condition).
 * @param predecessors Predecessor map, as created by graph traversal algorithms.
 * @param target Final vertex of the path.
 * @return Sequence of vertices, from source to target.
 * @pre Ensure that `predecessors[source] == source` for the source vertex used in the
 *      graph algorithm.
 */
PathBundle::path_type path_from_predecessors(
	std::vector<L1RoutingGraph::vertex_descriptor> const& predecessors,
	L1RoutingGraph::vertex_descriptor const& target);

} // namespace routing
} // namespace marocco
