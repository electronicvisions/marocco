#pragma once

#include <iostream>
#include <boost/operators.hpp>

#include "hal/Coordinate/L1.h"

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
	    HMF::Coordinate::HICANNOnWafer const& hicann, HMF::Coordinate::HLineOnHICANN const& hline);
	L1BusOnWafer(
	    HMF::Coordinate::HICANNOnWafer const& hicann, HMF::Coordinate::VLineOnHICANN const& vline);

	bool is_horizontal() const;

	bool is_vertical() const;

	HMF::Coordinate::Orientation toOrientation() const;

	HMF::Coordinate::HICANNOnWafer toHICANNOnWafer() const;

	/**
	 * @brief Returns the halbe coordinate corresponding to this bus.
	 * @pre You need to ensure that #is_horizontal() returns true before calling this function.
	 */
	HMF::Coordinate::HLineOnHICANN toHLineOnHICANN() const;

	/**
	 * @brief Returns the halbe coordinate corresponding to this bus.
	 * @pre You need to ensure that #is_vertical() returns true before calling this function.
	 */
	HMF::Coordinate::VLineOnHICANN toVLineOnHICANN() const;

	bool operator==(L1BusOnWafer const& other) const;

	/**
	 * @note This will be used by \c add_vertex().
	 */
	L1BusOnWafer();

private:
#ifndef PYPLUSPLUS
	friend class boost::serialization::access;
	template <typename Archiver>
	void serialize(Archiver& ar, unsigned int const /*version*/);
#endif // !PYPLUSPLUS

	HMF::Coordinate::HICANNOnWafer m_hicann;
	bool m_horizontal;
	size_t m_id;
}; // L1BusOnWafer

std::ostream& operator<<(std::ostream& os, L1BusOnWafer const& bus);

} // namespace routing
} // namespace marocco

#ifndef PYPLUSPLUS
#include <boost/serialization/export.hpp>
BOOST_CLASS_EXPORT_KEY(::marocco::routing::L1BusOnWafer)
#endif // !PYPLUSPLUS
