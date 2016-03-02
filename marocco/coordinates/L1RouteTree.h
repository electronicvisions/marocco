#pragma once

#include <iostream>
#include <boost/operators.hpp>
#include <boost/ref.hpp>
#ifndef PYPLUSPLUS
#include <memory>
#endif // !PYPLUSPLUS
#include <set>
#include <vector>

#include "pywrap/compat/macros.hpp"
#include "marocco/coordinates/L1Route.h"

namespace marocco {

/**
 * @brief L1 route with multiple targets, represented by a multi-branched tree.
 * @note To arrive at a deterministic layout for the tree, tails are sorted by their first
 *       route segment with empty routes coming last.  Segment types are sorted in the
 *       order specified by \c ::boost::variant::which(), i.e. their order in
 *       \c L1Route::segment_type.
 * @invariant There are no nullptrs in \c m_tails.
 */
class L1RouteTree : boost::equality_comparable<L1RouteTree>
{
public:
	L1RouteTree();
	L1RouteTree(L1RouteTree const& other);
	L1RouteTree& operator=(L1RouteTree const&);

	explicit L1RouteTree(L1Route const& route);

	/// Checks whether the tree is empty.
	bool empty() const;

	/// Returns a route containing those segments common to all targets.
	L1Route const& head() const;

	/// Checks whether this is a leaf node.
	bool has_tails() const;

	/**
	 * @brief Returns references to child nodes.
	 * @note An empty child node corresponds to a target reached en passant.
	 */
	std::vector<boost::reference_wrapper<L1RouteTree const> > tails() const;

	// Tails will be returned as copies instead of references in the python wrapping.
	// This hack is necessary so py++ registers the corresponding indexing_suite.
	PYPP_INSTANTIATE(std::vector<L1RouteTree>);

	/**
	 * @brief Adds a route to the tree.
	 * @note The tree is deterministic, i.e. the order and number of calls to \c add() do
	 *       not matter, when collecting multiple routes in a tree.
	 */
	void add(L1Route const& route);

	/**
	 * @brief Prepends an \c L1Route to the common head of the tree.
	 * @param mode Whether to call \c L1Route::extend() or \c L1Route::merge()
	 * @throw InvalidRouteError When this operation would lead to an invalid route.
	 */
	void prepend(L1Route const& other, L1Route::extend_mode mode);

	bool operator==(L1RouteTree const& other) const;

private:
#ifndef PYPLUSPLUS
	L1Route m_head;

	typedef std::unique_ptr<L1RouteTree> tail_type;
	struct TailCompare
	{
		bool operator()(tail_type const& lhs, tail_type const& rhs) const;
	};
	std::set<tail_type, TailCompare> m_tails;
#endif // !PYPLUSPLUS
};     // L1RouteTree

std::ostream& operator<<(std::ostream& os, L1RouteTree const& tree);

} // namespace marocco
