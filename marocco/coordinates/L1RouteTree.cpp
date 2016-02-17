#include "marocco/coordinates/L1RouteTree.h"

#include <boost/iterator/indirect_iterator.hpp>

namespace marocco {

L1RouteTree::L1RouteTree() : m_head()
{
}

L1RouteTree::L1RouteTree(L1RouteTree const& other) : m_head(other.m_head), m_tails()
{
	for (auto const& ptr : other.m_tails) {
		assert(ptr != nullptr);
		m_tails.insert(std::unique_ptr<L1RouteTree>(new L1RouteTree(*ptr)));
	}
}

L1RouteTree& L1RouteTree::operator=(L1RouteTree const& other)
{
	if (&other == this) {
		return *this;
	}
	m_head = other.m_head;
	m_tails.clear();
	for (auto const& ptr : other.m_tails) {
		assert(ptr != nullptr);
		m_tails.insert(std::unique_ptr<L1RouteTree>(new L1RouteTree(*ptr)));
	}
	return *this;
}

L1RouteTree::L1RouteTree(L1Route const& route) : m_head(route)
{
}

bool L1RouteTree::empty() const
{
	assert(m_head.empty() ? !has_tails() : true);
	return m_head.empty();
}

L1Route const& L1RouteTree::head() const
{
	return m_head;
}

bool L1RouteTree::has_tails() const
{
	return !m_tails.empty();
}

std::vector<boost::reference_wrapper<L1RouteTree const> > L1RouteTree::tails() const
{
	std::vector<boost::reference_wrapper<L1RouteTree const> > ret;
	ret.reserve(m_tails.size());
	for (auto const& ptr : m_tails) {
		ret.emplace_back(*ptr);
	}
	return ret;
}

bool L1RouteTree::operator==(L1RouteTree const& other) const
{
	if (m_head != other.m_head || m_tails.size() != other.m_tails.size()) {
		return false;
	}
	return std::equal(
		boost::make_indirect_iterator(m_tails.begin()),
		boost::make_indirect_iterator(m_tails.end()),
		boost::make_indirect_iterator(other.m_tails.begin()));
}

void L1RouteTree::add(L1Route const& route)
{
	auto const h_begin = m_head.begin(), h_end = m_head.end(), eit = route.end();
	auto h_it = h_begin, it = route.begin();

	// Skip segments common to both head and route.
	for (; it != eit && h_it != h_end; ++it, ++h_it) {
		if (!(*it == *h_it)) {
			break;
		}
	}

	if (h_begin == h_end) {
		assert(m_tails.empty());
		// If the tree is empty (default constructed), just replace head.
		m_head = route;
		// As empty trees are also used as 'en passant'-markers in leaf nodes, we have to
		// ensure that add() is never called on those.  As the API only allows const access
		// to tree branches the only place this could happen is inside this method.
		// (See recursive call below.)
		return;
	} else if (h_it == h_begin || h_it == std::next(h_begin)) {
		// `head` and `route` do not have any common segments but tree isn't empty.
		throw std::runtime_error("can not add disjunct route to non-empty tree");
	}

	L1Route first, second, h_second;
	std::tie(first, second) = route.split(it);
	std::tie(std::ignore, h_second) = m_head.split(h_it);

	if (h_it != h_end) {
		// `head` has segments that are not in `route` (or head ⊇ route).
		// Only the common part will be stored in this node.
		m_head = first;
		// Move segments remaining in head and current tails to new node.
		auto tail = std::unique_ptr<L1RouteTree>(new L1RouteTree(h_second));
		std::swap(m_tails, tail->m_tails);

		m_tails.insert(std::move(tail));
		m_tails.insert(std::unique_ptr<L1RouteTree>(new L1RouteTree(second)));
		return;
	}

	if (it != eit) {
		// All segments in `head` are also found in `route`,
		// but the two are not identical.
		assert(first == m_head);
		auto tail = std::unique_ptr<L1RouteTree>(new L1RouteTree(second));

		// Find tails that start with the same segment as `second`.  Note that we use a
		// custom set comparator that only checks the first segment (+ HICANN) of a route.
		auto it = m_tails.find(tail);

		if (it != m_tails.end()) {
			assert(*it != nullptr);
			// We should never call .add() on one of the 'en passant'-markers, as that
			// would lead to the replacement of their head.
			assert(!(*it)->empty());
			(*it)->add(second);
			return;
		}

		if (m_tails.empty()) {
			// Add an empty node to indicate that one route of the tree ends here.
			m_tails.insert(std::unique_ptr<L1RouteTree>(new L1RouteTree()));
		}
		m_tails.insert(std::move(tail));
	}
}

bool L1RouteTree::TailCompare::operator()(tail_type const& lhs, tail_type const& rhs) const
{
	assert(lhs != nullptr && rhs != nullptr);
	auto const& left_segments = lhs->head().segments();
	auto const& right_segments = rhs->head().segments();
	if (left_segments.empty() || right_segments.empty()) {
		return left_segments.empty() < right_segments.empty();
	}
	if (left_segments[0] == right_segments[0]) {
		return left_segments[1] < right_segments[1];
	}
	return left_segments[0] < right_segments[0];
}

} // namespace marocco
