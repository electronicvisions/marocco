#include "marocco/coordinates/L1Route.h"
#include "marocco/coordinates/printers.h"

#include <sstream>
#include <boost/variant/static_visitor.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/variant.hpp>

#include "hal/HICANN/Crossbar.h"
#include "hal/HICANN/SynapseSwitch.h"

namespace marocco {

using namespace HMF::Coordinate;

bool is_hicann(L1Route::segment_type const& segment) {
	return boost::get<HICANNOnWafer>(&segment) != nullptr;
}

class IsValidSuccessor : public boost::static_visitor<bool>
{
	HICANNOnWafer m_current_hicann;
	// Previous segment when crossing HICANN boundaries.
	L1Route::segment_type m_previous_segment;

public:
	IsValidSuccessor(HICANNOnWafer hicann) : m_current_hicann(std::move(hicann))
	{
	}

	HICANNOnWafer current_hicann() const
	{
		return m_current_hicann;
	}

	L1Route::iterator find_invalid(L1Route::iterator it, L1Route::iterator const end)
	{
		auto next = std::next(it);

		for (; next != end; ++it, ++next) {
			if (!boost::apply_visitor(*this, *it, *next)) {
				return next;
			}
		}

		return end;
	}

	// The following encodes all possible transitions / pairs of segments.

	// Every pair that is not explicitly specified is not allowed.
	template <typename T, typename V>
	bool operator()(T const& /*current*/, V const& /*next*/)
	{
		return false;
	}

	//  ——— HICANN boundaries ——————————————————————————————————————————————————

	bool operator()(HLineOnHICANN const& current, HICANNOnWafer const& /*next*/)
	{
		m_previous_segment = current;
		return true;
	}

	bool operator()(VLineOnHICANN const& current, HICANNOnWafer const& /*next*/)
	{
		m_previous_segment = current;
		return true;
	}

	// "output to the left" case of sending repeater
	bool operator()(DNCMergerOnHICANN const& current, HICANNOnWafer const& next)
	{
		if (m_current_hicann.x() <= next.x()) {
			return false;
		}
		return operator()(current.toSendingRepeaterOnHICANN().toHLineOnHICANN(), next);
	}

	bool operator()(HICANNOnWafer const& hicann, HLineOnHICANN const& next)
	{
		int diff = int(m_current_hicann.x()) - hicann.x();
		if (diff == 0) {
			return false;
		}
		auto const* previous = boost::get<HLineOnHICANN>(&m_previous_segment);
		if (previous == nullptr || next != (diff < 0 ? previous->east() : previous->west())) {
			return false;
		}
		m_current_hicann = hicann;
		return true;
	}

	bool operator()(HICANNOnWafer const& hicann, VLineOnHICANN const& next)
	{
		int diff = int(m_current_hicann.y()) - hicann.y();
		if (diff == 0) {
			return false;
		}
		auto const* previous = boost::get<VLineOnHICANN>(&m_previous_segment);
		if (previous == nullptr || next != (diff < 0 ? previous->south() : previous->north())) {
			return false;
		}
		m_current_hicann = hicann;
		return true;
	}

	// Adjacent input to synapse array.
	bool operator()(HICANNOnWafer const& hicann, SynapseDriverOnHICANN const& next)
	{
		int diff = int(m_current_hicann.x()) - hicann.x();
		if (m_current_hicann.y() != hicann.y()) {
			return false;
		}
		auto const* previous = boost::get<VLineOnHICANN>(&m_previous_segment);
		if (previous == nullptr || previous->toSideHorizontal() == next.toSideHorizontal() ||
		    diff != (previous->toSideHorizontal() == left ? 1 : -1)) {
			return false;
		}
		m_current_hicann = hicann;
		return true;
	}

	//  ——— Merger tree ————————————————————————————————————————————————————————

	bool operator()(Merger0OnHICANN const& current, Merger1OnHICANN const& next)
	{
		size_t id = current.value();
		size_t next_id = next.value();
		// clang-format off
		return (
			(next_id == 0u && id < 2) ||
			(next_id == 1u && id >= 2 && id < 4) ||
			(next_id == 2u && id >= 4 && id < 6) ||
			(next_id == 3u && id >= 6));
		// clang-format on
	}

	bool operator()(Merger1OnHICANN const& current, Merger2OnHICANN const& next)
	{
		size_t id = current.value();
		size_t next_id = next.value();
		// clang-format off
		return (
			(next_id == 0u && id < 2) ||
			(next_id == 1u && id >= 2));
		// clang-format on
	}

	bool operator()(Merger2OnHICANN const& /*current*/, Merger3OnHICANN const& /*next*/)
	{
		return true;
	}

	bool operator()(Merger0OnHICANN const& current, DNCMergerOnHICANN const& next)
	{
		size_t id = current.value();
		return ((id == next.value()) && (id == 0u || id == 2u || id == 4u || id == 7u));
	}

	bool operator()(Merger1OnHICANN const& current, DNCMergerOnHICANN const& next)
	{
		return (
		    (current.value() == 0u && next.value() == 1u) ||
		    (current.value() == 3u && next.value() == 6u));
	}

	bool operator()(Merger2OnHICANN const& current, DNCMergerOnHICANN const& next)
	{
		return current.value() == 1u && next.value() == 5u;
	}

	bool operator()(Merger3OnHICANN const& /*current*/, DNCMergerOnHICANN const& next)
	{
		return next.value() == 3u;
	}

	bool operator()(DNCMergerOnHICANN const& current, GbitLinkOnHICANN const& next)
	{
		// GbitlinkOnHICANN in out-configuration.
		return current.value() == next.value();
	}

	bool operator()(GbitLinkOnHICANN const& current, DNCMergerOnHICANN const& next)
	{
		// GbitlinkOnHICANN in in-configuration.
		return current.value() == next.value();
	}

	//  ——— Test ports —————————————————————————————————————————————————————————

	bool operator()(RepeaterBlockOnHICANN const& current, HLineOnHICANN const& next)
	{
		return current == next.toHRepeaterOnHICANN().toRepeaterBlockOnHICANN();
	}

	bool operator()(RepeaterBlockOnHICANN const& current, VLineOnHICANN const& next)
	{
		return current == next.toVRepeaterOnHICANN().toRepeaterBlockOnHICANN();
	}

	//  ——— L1 buses ———————————————————————————————————————————————————————————

	bool operator()(DNCMergerOnHICANN const& current, HLineOnHICANN const& next)
	{
		return current.toSendingRepeaterOnHICANN().toHLineOnHICANN() == next;
	}

	bool operator()(VLineOnHICANN const& current, HLineOnHICANN const& next)
	{
		return HMF::HICANN::Crossbar::exists(current, next);
	}

	bool operator()(HLineOnHICANN const& current, VLineOnHICANN const& next)
	{
		return HMF::HICANN::Crossbar::exists(next, current);
	}

	//  ——— Synapse drivers ————————————————————————————————————————————————————

	bool operator()(VLineOnHICANN const& current, SynapseDriverOnHICANN const& next)
	{
		return HMF::HICANN::SynapseSwitch::exists(current, next.toSynapseSwitchRowOnHICANN().y());
	}

	bool operator()(SynapseDriverOnHICANN const& current, SynapseDriverOnHICANN const& next)
	{
		// Only adjacent synapse drivers can be chained.
		return current.x() == next.x() && std::abs(int(current.y()) - int(next.y())) == 2;
	}

	bool operator()(SynapseDriverOnHICANN const& /*current*/, SynapseOnHICANN const& /*next*/)
	{
		return true;
	}
};

bool L1Route::empty() const
{
	return m_segments.empty();
}

HICANNOnWafer const& L1Route::source_hicann() const
{
	if (m_segments.empty()) {
		throw std::runtime_error("source_hicann() called on empty route");
	}

	if (auto const* hicann = boost::get<HICANNOnWafer>(&m_segments.front())) {
		return *hicann;
	}

	throw std::logic_error("route does not start with HICANNOnWafer");
}

HICANNOnWafer const& L1Route::target_hicann() const
{
	if (m_segments.empty()) {
		throw std::runtime_error("target_hicann() called on empty route");
	}

	return m_last_hicann;
}

auto L1Route::front() const -> segment_type const&
{
	if (m_segments.empty()) {
		throw std::runtime_error("front() called on empty route");
	}

	return *std::next(m_segments.begin());
}

auto L1Route::back() const -> segment_type const&
{
	if (m_segments.empty()) {
		throw std::runtime_error("back() called on empty route");
	}

	return m_segments.back();
}

auto L1Route::segments() const -> sequence_type const&
{
	return m_segments;
}

size_t L1Route::size() const
{
	return m_segments.size();
}

auto L1Route::begin() const -> iterator
{
	return m_segments.begin();
}

auto L1Route::end() const -> iterator
{
	return m_segments.end();
}

auto L1Route::operator[](size_t pos) const -> segment_type const&
{
	return m_segments.at(pos);
}

void L1Route::append(segment_type const& segment)
{
	if (is_hicann(segment)) {
		throw InvalidRouteError("can not add HICANNOnWafer on its own");
	}

	if (m_segments.empty()) {
		throw InvalidRouteError("route has to start with HICANNOnWafer");
	}

	auto hicann = target_hicann();
	IsValidSuccessor visitor(hicann);
	if (!boost::apply_visitor(visitor, m_segments.back(), segment)) {
		std::ostringstream err;
		err << "trying to insert invalid segment: " << segment;
		throw InvalidRouteError(err.str());
	}

	m_segments.push_back(segment);
}

void L1Route::append(HICANNOnWafer const& hicann, segment_type const& segment)
{
	if (is_hicann(segment)) {
		throw InvalidRouteError("can not add two consecutive HICANNOnWafers");
	}

	bool empty = m_segments.empty();

	if (!empty && hicann == m_last_hicann) {
		return append(segment);
	}

	IsValidSuccessor visitor(m_last_hicann);

	segment_type hicann_segment{hicann};

	if (!empty && (!boost::apply_visitor(visitor, m_segments.back(), hicann_segment) ||
	               !boost::apply_visitor(visitor, hicann_segment, segment))) {
		std::ostringstream err;
		err << "trying to insert invalid segment: " << segment;
		throw InvalidRouteError(err.str());
	}

	m_last_hicann = hicann;
	m_segments.emplace_back(std::move(hicann_segment));
	m_segments.push_back(segment);
}

void L1Route::append(L1Route const& other, extend_mode mode)
{
	switch (mode) {
		case extend_mode::extend:
			extend_impl(other);
			break;
		case extend_mode::merge_common_endpoints:
			merge_impl(other);
			break;
		default:
			break;
	}
}

void L1Route::prepend(L1Route const& other, extend_mode mode)
{
	L1Route route(other);
	route.append(*this, mode);
	*this = route;
}

void L1Route::extend_impl(L1Route const& other)
{
	if (empty()) {
		*this = other;
		return;
	} else if (other.empty()) {
		return;
	}

	IsValidSuccessor visitor(m_last_hicann);
	auto hicann = other.source_hicann();
	auto const& last_segment = m_segments.back();

	auto it = other.m_segments.begin();

	if (hicann == m_last_hicann && boost::apply_visitor(visitor, last_segment, *std::next(it))) {
		++it;
	} else if (
		// We need to look at three consecutive elements when crossing HICANN boundaries
		// (one before, one after a HICANNOnWafer coordinate) so the visitor can calculate
		// the expected L1 bus and check it.
	    !(hicann != m_last_hicann && boost::apply_visitor(visitor, last_segment, *it) &&
	      boost::apply_visitor(visitor, *it, *std::next(it)))) {
		std::ostringstream err;
		err << "invalid starting segment when extending:\n  [..., " << last_segment << "] + ["
		    << *it << ", " << *std::next(it) << ", ...]";
		throw InvalidRouteError(err.str());
	}

	std::copy(it, other.m_segments.end(), std::back_inserter(m_segments));
	m_last_hicann = other.target_hicann();
}

void L1Route::merge_impl(L1Route const& other)
{
	if (empty()) {
		*this = other;
		return;
	} else if (other.empty()) {
		return;
	}

	auto const& last_segment = m_segments.back();
	auto hicann = other.source_hicann();
	auto it = std::next(other.m_segments.begin());
	if (m_last_hicann != hicann) {
		std::ostringstream err;
		err << "invalid source HICANN when merging: " << hicann << ", expected " << m_last_hicann;
		throw InvalidRouteError(err.str());
	} else if (!(last_segment == *it)) {
		std::ostringstream err;
		err << "invalid starting segment when merging:\n  [..., " << last_segment << "] + ["
		    << *std::prev(it) << ", " << *it << ", ...]";
		throw InvalidRouteError(err.str());
	}

	std::copy(std::next(it), other.m_segments.end(), std::back_inserter(m_segments));
	m_last_hicann = other.target_hicann();
}

std::pair<L1Route, L1Route> L1Route::split(iterator it) const
{
	if (it == m_segments.begin()) {
		return std::make_pair(L1Route(), *this);
	} else if (it == m_segments.end()) {
		return std::make_pair(*this, L1Route());
	}

	if (is_hicann(*std::prev(it))) {
		--it;
	}

	sequence_type first(m_segments.begin(), it);
	sequence_type second;
	second.reserve(size() - first.size() + 1);

	if (!is_hicann(*it)) {
		sequence_type::const_reverse_iterator rit{it};
		rit = std::find_if(rit, m_segments.rend(), is_hicann);
		assert(rit != m_segments.rend());
		second.push_back(*rit);
	}

	std::copy(it, m_segments.end(), std::back_inserter(second));

	return std::make_pair(
	    L1Route(std::move(first), no_verify_tag()), L1Route(std::move(second), no_verify_tag()));
}

bool L1Route::operator==(L1Route const& other) const {
	return m_segments == other.m_segments;
}

L1Route::L1Route() : m_segments()
{
}

L1Route::L1Route(sequence_type&& segments) : m_segments(std::move(segments))
{
	verify();
}

L1Route::L1Route(sequence_type&& segments, no_verify_tag) : m_segments(std::move(segments))
{
	update_target_hicann();
}

L1Route::L1Route(sequence_type const& segments) : m_segments(segments)
{
	verify();
}

L1Route::L1Route(sequence_type const& segments, no_verify_tag) : m_segments(segments)
{
	update_target_hicann();
}

L1Route::L1Route(std::initializer_list<segment_type> segments) : m_segments(segments)
{
	verify();
}

void L1Route::verify() {
	auto beg = m_segments.cbegin();
	auto end = m_segments.cend();
	auto it = find_invalid(beg, end, &m_last_hicann);
	if (it != end) {
		std::ostringstream err;
		err << "invalid segment in route: " << *it << " at index " << std::distance(beg, it);
		throw InvalidRouteError(err.str());
	}
}

void L1Route::update_target_hicann()
{
	for (auto rit = m_segments.rbegin(); rit != m_segments.rend(); ++rit) {
		if (auto* hicann = boost::get<HICANNOnWafer>(&*rit)) {
			m_last_hicann = *hicann;
			return;
		}
	}
}

auto
L1Route::find_invalid(
    iterator const beg, iterator const end, HMF::Coordinate::HICANNOnWafer* store_last_hicann)
    -> iterator
{
	if (beg == end) {
		// empty route
		return end;
	}

	// HICANNOnWafer is required as first element.
	auto const* starting_hicann = boost::get<HICANNOnWafer>(&*beg);

	// Ensure that HICANN + at least one other segment are present.
	if (starting_hicann == nullptr || std::next(beg) == end) {
		return beg;
	}

	// Ensure that route does not end with a HICANNOnWafer segment.
	auto const last = std::prev(end);
	if (boost::get<HICANNOnWafer>(&*last) != nullptr) {
		return last;
	}

	IsValidSuccessor visitor(*starting_hicann);
	auto it = visitor.find_invalid(std::next(beg), end);

	if (it == end && store_last_hicann != nullptr) {
		*store_last_hicann = visitor.current_hicann();
	}

	return it;
}

std::ostream& operator<<(std::ostream& os, L1Route const& route)
{
	return os << pretty_printed(route);
}

template <typename Archiver>
void L1Route::serialize(Archiver& ar, unsigned int const /*version*/)
{
	using namespace boost::serialization;
	// clang-format off
	ar & make_nvp("last_hicann", m_last_hicann)
	   & make_nvp("segments", m_segments);
	// clang-format on
}

} // namespace marocco

BOOST_CLASS_EXPORT_IMPLEMENT(::marocco::L1Route)

#include "boost/serialization/serialization_helper.tcc"
EXPLICIT_INSTANTIATE_BOOST_SERIALIZE(::marocco::L1Route)
