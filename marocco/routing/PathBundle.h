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

} // namespace routing
} // namespace marocco
