#pragma once

#include <iostream>
#include <boost/operators.hpp>

#include "halco/hicann/v2/hicann.h"
#include "halco/hicann/v2/l1.h"
#include "halco/common/relations.h"

namespace marocco {
namespace routing {

/**
 * @brief Represents a horizontal or vertical L1 bus on a wafer.
 * @see L1RoutingGraph, where this is used as the vertex type.
 */
class L1BusOnWafer : boost::equality_comparable<L1BusOnWafer>
{
public:
	L1BusOnWafer(
	    halco::hicann::v2::HICANNOnWafer const& hicann, halco::hicann::v2::HLineOnHICANN const& hline);
	L1BusOnWafer(
	    halco::hicann::v2::HICANNOnWafer const& hicann, halco::hicann::v2::VLineOnHICANN const& vline);

	bool is_horizontal() const;

	bool is_vertical() const;

	halco::common::Orientation toOrientation() const;

	halco::hicann::v2::HICANNOnWafer toHICANNOnWafer() const;

	/**
	 * @brief Returns the halbe coordinate corresponding to this bus.
	 * @pre You need to ensure that #is_horizontal() returns true before calling this function.
	 */
	halco::hicann::v2::HLineOnHICANN toHLineOnHICANN() const;

	/**
	 * @brief Returns the halbe coordinate corresponding to this bus.
	 * @pre You need to ensure that #is_vertical() returns true before calling this function.
	 */
	halco::hicann::v2::VLineOnHICANN toVLineOnHICANN() const;

	bool operator==(L1BusOnWafer const& other) const;

	/**
	 * @note This will be used by \c add_vertex().
	 */
	L1BusOnWafer();

	size_t hash() const;

private:
#ifndef PYPLUSPLUS
	friend class boost::serialization::access;
	template <typename Archiver>
	void serialize(Archiver& ar, unsigned int const /*version*/);
#endif // !PYPLUSPLUS

	halco::hicann::v2::HICANNOnWafer m_hicann;
	bool m_horizontal;
	size_t m_id;
}; // L1BusOnWafer

std::ostream& operator<<(std::ostream& os, L1BusOnWafer const& bus);


size_t hash_value(L1BusOnWafer const& bus);

} // namespace routing
} // namespace marocco

namespace std {

template <>
struct hash<marocco::routing::L1BusOnWafer>
{
	size_t operator()(marocco::routing::L1BusOnWafer const& bus) const { return hash_value(bus); }
};

} // namespace std

#ifndef PYPLUSPLUS
#include <boost/serialization/export.hpp>
BOOST_CLASS_EXPORT_KEY(::marocco::routing::L1BusOnWafer)
#endif // !PYPLUSPLUS
