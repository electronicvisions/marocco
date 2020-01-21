#pragma once

#include <iostream>
#include <stdexcept>
#include <vector>
#include <boost/operators.hpp>
#include <boost/serialization/export.hpp>
#include <boost/variant.hpp>

#include "halco/hicann/v2/external.h"
#include "halco/hicann/v2/hicann.h"
#include "halco/hicann/v2/l1.h"
#include "halco/hicann/v2/merger0onhicann.h"
#include "halco/hicann/v2/merger1onhicann.h"
#include "halco/hicann/v2/merger2onhicann.h"
#include "halco/hicann/v2/merger3onhicann.h"
#include "halco/hicann/v2/synapse.h"
#include "pywrap/compat/macros.hpp"
#include "marocco/test.h"

namespace boost {
namespace serialization {
class access;
} // namespace serialization
} // namespace boost

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
	typedef boost::variant<halco::hicann::v2::HICANNOnWafer,
	                       halco::hicann::v2::Merger0OnHICANN,
	                       halco::hicann::v2::Merger1OnHICANN,
	                       halco::hicann::v2::Merger2OnHICANN,
	                       halco::hicann::v2::Merger3OnHICANN,
	                       halco::hicann::v2::GbitLinkOnHICANN,

	                       // Events from merger tree/external input,
	                       // to be sent via sending repeater.
	                       halco::hicann::v2::DNCMergerOnHICANN,

	                       // Events from test port on repeater block,
	                       // can be sent to bus via corresponding repeater.
	                       halco::hicann::v2::RepeaterBlockOnHICANN,

	                       halco::hicann::v2::HLineOnHICANN,
	                       halco::hicann::v2::VLineOnHICANN,
	                       halco::hicann::v2::SynapseDriverOnHICANN,
	                       halco::hicann::v2::SynapseOnHICANN>
	    segment_type;
	typedef std::vector<segment_type> sequence_type;
	typedef sequence_type::const_iterator iterator;

	struct no_verify_tag
	{
	};

	PYPP_CLASS_ENUM(extend_mode)
	{
		extend,
		merge_common_endpoints
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
	typedef size_t size_type;
	size_type size() const;

	/// Returns an iterator to the beginning.
	iterator begin() const;

	/// Returns an iterator to the beginning.
	iterator end() const;

	/// Returns specified element of route.
	typedef segment_type value_type;
	segment_type const& operator[](size_t pos) const;

	/// Returns the first segment of the route that is not a \c HICANNOnWafer.
	segment_type const& front() const;

	/// Returns the last segment of the route that is not a \c HICANNOnWafer.
	segment_type const& back() const;

	/// Returns the \c HICANNOnWafer this route starts from.
	halco::hicann::v2::HICANNOnWafer const& source_hicann() const;

	/// Returns the \c HICANNOnWafer this route leads to.
	halco::hicann::v2::HICANNOnWafer const& target_hicann() const;

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
	void append(halco::hicann::v2::HICANNOnWafer const& hicann, segment_type const& segment);

	/**
	 * @brief Extends this route by appending the segments contained in another route.
	 * @throw InvalidRouteError When the other route is not a valid extension of this route.
	 * @param mode When \c extend_mode::extend is supplied, this handles the case
	 *             \c [ABC]+[DE]=[ABCDE], thus requiring validation of the succession \c C→D.
	 *             When \c extend_mode::merge_common_endpoints is supplied, this handles
	 *             the case \c [ABC]+[CDE]=[ABCDE], thus requiring fewer checks compared
	 *             to \c extend_mode::extend.
	 */
	void append(L1Route const& other, extend_mode mode);

	/**
	 * @brief Extends this route by prepending the segments contained in another route.
	 * @see \c append() for parameters and possible exceptions.
	 */
	void prepend(L1Route const& other, extend_mode mode);

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

	void extend_impl(L1Route const& other);
	void merge_impl(L1Route const& other);

	FRIEND_TEST(L1Route, findInvalid);

	static iterator find_invalid(
	    iterator beg, iterator end, halco::hicann::v2::HICANNOnWafer* store_last_hicann = nullptr);

	halco::hicann::v2::HICANNOnWafer m_last_hicann;
	sequence_type m_segments;

	friend class boost::serialization::access;
	template <typename Archiver>
	void serialize(Archiver& ar, unsigned int const /*version*/);
}; // L1Route

std::ostream& operator<<(std::ostream& os, L1Route const& route);

} // namespace marocco

BOOST_CLASS_EXPORT_KEY(::marocco::L1Route)
