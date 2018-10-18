#pragma once

#include <iostream>
#include <boost/functional/hash.hpp>
#include <boost/operators.hpp>
#include <boost/serialization/serialization.hpp>

#include "hal/Coordinate/Wafer.h"
#include "marocco/routing/L1BusOnWafer.h"

namespace marocco {
namespace routing {

/**
 * @brief Represents a horizontal or vertical L1 bus with a wafer.
 */
class L1BusGlobal : boost::equality_comparable<L1BusGlobal>
{
public:
	L1BusGlobal(L1BusOnWafer const& bus, HMF::Coordinate::Wafer const& wafer);
	L1BusGlobal(){};

	HMF::Coordinate::Wafer toWafer() const;
	L1BusOnWafer toL1BusOnWafer() const;

	bool operator==(L1BusGlobal const& other) const;


private:
#ifndef PYPLUSPLUS
	friend class boost::serialization::access;
	template <typename Archiver>
	void serialize(Archiver& ar, unsigned int const /*version*/);
#endif // !PYPLUSPLUS

	L1BusOnWafer m_bus;
	HMF::Coordinate::Wafer m_wafer;
}; // L1BusGlobal

std::ostream& operator<<(std::ostream& os, L1BusGlobal const& bus);

} // namespace routing
} // namespace marocco

namespace std {

template <>
struct hash<marocco::routing::L1BusGlobal>
{
	size_t operator()(marocco::routing::L1BusGlobal const& bus) const
	{
		size_t hash = 0;
		boost::hash_combine(hash, bus.toWafer());
		boost::hash_combine(hash, bus.toL1BusOnWafer());
		return hash;
	}
};

} // namespace std

#ifndef PYPLUSPLUS
#include <boost/serialization/export.hpp>
BOOST_CLASS_EXPORT_KEY(::marocco::routing::L1BusGlobal)
#endif // !PYPLUSPLUS
