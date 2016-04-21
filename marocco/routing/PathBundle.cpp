#include "marocco/routing/PathBundle.h"

namespace marocco {
namespace routing {

namespace {

PathBundle::vertex_descriptor target_from_path(PathBundle::path_type const& path)
{
	return path.back();
}

} // namespace

PathBundle::PathBundle() : m_paths()
{
}

PathBundle::PathBundle(path_type const& path) : m_paths{path}
{
	if (path.empty()) {
		throw std::invalid_argument("path can not be empty");
	}
}

PathBundle::PathBundle(bundle_type const& paths) : m_paths(paths)
{
	for (auto const& path : m_paths) {
		if (path.empty()) {
			throw std::invalid_argument("path can not be empty");
		}
		if (path.front() != source()) {
			throw std::invalid_argument("paths do not have common starting point");
		}
	}
}

void PathBundle::add(path_type const& path)
{
	if (path.empty()) {
		throw std::invalid_argument("path can not be empty");
	}
	if (!empty() && path.front() != source()) {
		throw std::invalid_argument("path does not start at starting point of bundle");
	}
	m_paths.push_back(path);
}

auto PathBundle::source() const -> vertex_descriptor
{
	if (empty()) {
		throw std::runtime_error("empty bundle does not have a source");
	}
	return m_paths.front().front();
}

auto PathBundle::targets() const -> iterable<vertex_iterator>
{
	vertex_from_bundle_type conv(&target_from_path);
	return make_iterable(
		boost::make_transform_iterator(m_paths.begin(), conv),
		boost::make_transform_iterator(m_paths.end(), conv));
}

auto PathBundle::paths() const -> bundle_type const&
{
	return m_paths;
}

size_t PathBundle::size() const
{
	return m_paths.size();
}

bool PathBundle::empty() const
{
	return m_paths.empty();
}

PathBundle::path_type path_from_predecessors(
	std::vector<L1RoutingGraph::vertex_descriptor> const& predecessors,
	L1RoutingGraph::vertex_descriptor const& target)
{
	PathBundle::path_type path;

	auto current = target;
	while (true) {
		path.push_back(current);

		auto previous = predecessors[current];
		if (previous == current) {
			break;
		}

		current = previous;
	}
	std::reverse(path.begin(), path.end());

	return path;
}

} // namespace routing
} // namespace marocco
