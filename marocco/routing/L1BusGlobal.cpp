#include "marocco/routing/L1BusGlobal.h"

#include <boost/serialization/nvp.hpp>

#include "hal/Coordinate/Wafer.h"
#include "marocco/routing/L1BusOnWafer.h"

namespace marocco {
namespace routing {

L1BusGlobal::L1BusGlobal(L1BusOnWafer const& bus, HMF::Coordinate::Wafer const& wafer)
    : m_bus(bus), m_wafer(wafer)
{}

L1BusOnWafer L1BusGlobal::toL1BusOnWafer() const
{
	return m_bus;
}

HMF::Coordinate::Wafer L1BusGlobal::toWafer() const
{
	return m_wafer;
}

bool L1BusGlobal::operator==(L1BusGlobal const& other) const
{
	return m_bus == other.toL1BusOnWafer() && m_wafer == other.toWafer();
}

std::ostream& operator<<(std::ostream& os, L1BusGlobal const& bus)
{
	os << "L1BusGlobal(" << bus.toL1BusOnWafer() << ", ";
	os << bus.toWafer();
	os << ")";
	return os;
}

template <typename Archiver>
void L1BusGlobal::serialize(Archiver& ar, unsigned int const /*version*/)
{
	using namespace boost::serialization;
	ar& make_nvp("wafer", m_wafer) & make_nvp("l1bus", m_bus);
}

} // namespace routing
} // namespace marocco

BOOST_CLASS_EXPORT_IMPLEMENT(::marocco::routing::L1BusGlobal)

#include "boost/serialization/serialization_helper.tcc"
EXPLICIT_INSTANTIATE_BOOST_SERIALIZE(::marocco::routing::L1BusGlobal)
