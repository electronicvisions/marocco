#pragma once

#include <boost/operators.hpp>
#include <boost/variant.hpp>
#include <vector>
#include <stdexcept>

#include "hal/Coordinate/External.h"
#include "hal/Coordinate/L1.h"
#include "hal/Coordinate/Merger0OnHICANN.h"
#include "hal/Coordinate/Merger1OnHICANN.h"
#include "hal/Coordinate/Merger2OnHICANN.h"
#include "hal/Coordinate/Merger3OnHICANN.h"
#include "hal/Coordinate/Synapse.h"
#include "marocco/test.h"

namespace marocco {

class InvalidRouteError : public std::runtime_error
{
public:
	InvalidRouteError(const std::string& what) : std::runtime_error(what)
	{
	}
};

class L1Route : public boost::equality_comparable<L1Route>
{
public:
	typedef boost::variant<HMF::Coordinate::HICANNOnWafer,
	                       HMF::Coordinate::Merger0OnHICANN,
	                       HMF::Coordinate::Merger1OnHICANN,
	                       HMF::Coordinate::Merger2OnHICANN,
	                       HMF::Coordinate::Merger3OnHICANN,
	                       HMF::Coordinate::GbitLinkOnHICANN,
	                       HMF::Coordinate::DNCMergerOnHICANN,
	                       HMF::Coordinate::HLineOnHICANN,
	                       HMF::Coordinate::VLineOnHICANN,
	                       HMF::Coordinate::SynapseDriverOnHICANN,
	                       HMF::Coordinate::SynapseOnHICANN>
	    segment_type;
	typedef std::vector<segment_type> sequence_type;
	typedef sequence_type::const_iterator iterator;

	struct no_verify_tag
	{
	};

	L1Route();
#ifndef PYPLUSPLUS
	L1Route(sequence_type&& segments);
	L1Route(sequence_type&& segments, no_verify_tag);
	L1Route(sequence_type const& segments);
	L1Route(sequence_type const& segments, no_verify_tag);
	L1Route(std::initializer_list<segment_type> segments);
#endif // !PYPLUSPLUS

	/// Checks whether the route is empty.
	bool empty() const;

#ifndef PYPLUSPLUS
	/**
	 * @brief Returns all route segments including \c HICANNOnWafer coordinates, which
	 *        encode changes between chips.
	 * @note A \c HICANNOnWafer indicates the current chip for all following segments
	 *       until the next \c HICANNOnWafer.
	 */
	// [py:return-internal-reference]
	sequence_type const& segments() const;
#endif // !PYPLUSPLUS

	/// Returns the number of route segments.
	size_t size() const;

	/// Returns an iterator to the beginning.
	iterator begin() const;

	/// Returns an iterator to the beginning.
	iterator end() const;

	/// Returns specified element of route.
	segment_type const& operator[](size_t pos) const;

	/// Returns the first segment of the route that is not a \c HICANNOnWafer.
	segment_type front() const;

	/// Returns the last segment of the route that is not a \c HICANNOnWafer.
	segment_type back() const;

	/// Returns the \c HICANNOnWafer this route starts from.
	HMF::Coordinate::HICANNOnWafer source_hicann() const;

	/// Returns the \c HICANNOnWafer this route leads to.
	HMF::Coordinate::HICANNOnWafer target_hicann() const;

	/**
	 * @brief Extends this route by a single segment.
	 * @param segment Coordinate of segment to be inserted.  Can not be of type
	 *                \c HICANNOnWafer.
	 * @note Segments on a different chip have to be introduced using the overload of this
	 *       function with two arguments.
	 * @throw InvalidRouteError When the segment can not be inserted at this position.
	 */
	void append(segment_type const& segment);

	/**
	 * @brief Extends this route by a single segment on a (potentially) different chip.
	 * @param hicann \c HICANNOnWafer of the following segment.
	 * @param segment Coordinate of segment to be inserted.  Can not be of type
	 *                \c HICANNOnWafer.
	 * @throw InvalidRouteError When the segment can not be inserted at this position.
	 * @note This just dispatches to the one-argument version of #append() when passing in
	 *       the current \c HICANNOnWafer, so you may prefer to directly use that function.
	 */
	void append(HMF::Coordinate::HICANNOnWafer const& hicann, segment_type const& segment);

	/**
	 * @brief Extends this route by the segments contained in another route.
	 * @throw InvalidRouteError When the other route is not a valid extension of this route.
	 * @note This handles the case \c [ABC]+[DE]=[ABCDE], thus requiring validation of the
	 *       succession \c C→D.
	 */
	void extend(L1Route const& other);

	/**
	 * @brief Merges another route with common endpoint into this route.
	 * @throw InvalidRouteError When either route is empty or the other route does not start
	 *        at the last segment of the current route.
	 * @note This only handles the case \c [ABC]+[CDE]=[ABCDE], thus requiring fewer
	 *       checks compared to \c extend().
	 */
	void merge(L1Route const& other);

#ifndef PYPLUSPLUS
	/**
	 * @brief Returns the result of splitting this route as two valid \c L1Route objects.
	 * @param it Iterator to the first segment that should be contained in the second route.
	 */
	std::pair<L1Route, L1Route> split(iterator it) const;
#endif // !PYPLUSPLUS

	bool operator==(L1Route const& other) const;

private:
	void verify();
	void update_target_hicann();

	FRIEND_TEST(L1Route, findInvalid);

	static iterator find_invalid(
	    iterator beg, iterator end, HMF::Coordinate::HICANNOnWafer* store_last_hicann = nullptr);

	HMF::Coordinate::HICANNOnWafer m_last_hicann;
	sequence_type m_segments;
}; // L1Route

} // namespace marocco
