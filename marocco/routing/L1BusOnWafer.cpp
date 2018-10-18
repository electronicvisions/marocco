#include "marocco/routing/L1BusOnWafer.h"

#include <boost/serialization/nvp.hpp>

using namespace HMF::Coordinate;

namespace marocco {
namespace routing {


L1BusOnWafer::L1BusOnWafer() : m_hicann(), m_horizontal(true), m_id()
{
}

L1BusOnWafer::L1BusOnWafer(HICANNOnWafer const& hicann, HLineOnHICANN const& hline)
	: m_hicann(hicann), m_horizontal(true), m_id(hline.value())
{
}

L1BusOnWafer::L1BusOnWafer(HICANNOnWafer const& hicann, VLineOnHICANN const& vline)
	: m_hicann(hicann), m_horizontal(false), m_id(vline.value())
{
}

HICANNOnWafer L1BusOnWafer::toHICANNOnWafer() const
{
	return m_hicann;
}

HLineOnHICANN L1BusOnWafer::toHLineOnHICANN() const
{
	assert(is_horizontal());
	return HLineOnHICANN(m_id);
}

VLineOnHICANN L1BusOnWafer::toVLineOnHICANN() const
{
	assert(is_vertical());
	return VLineOnHICANN(m_id);
}

Orientation L1BusOnWafer::toOrientation() const
{
	return m_horizontal ? horizontal : vertical;
}

bool L1BusOnWafer::is_horizontal() const
{
	return m_horizontal;
}

bool L1BusOnWafer::is_vertical() const
{
	return !m_horizontal;
}

bool L1BusOnWafer::operator==(L1BusOnWafer const& other) const
{
	return m_hicann == other.m_hicann && m_horizontal == other.m_horizontal && m_id == other.m_id;
}

std::ostream& operator<<(std::ostream& os, L1BusOnWafer const& bus)
{
	os << "L1BusOnWafer(" << bus.toHICANNOnWafer() << ", ";
	if (bus.is_horizontal()) {
		os << bus.toHLineOnHICANN();
	} else {
		os << bus.toVLineOnHICANN();
	}
	os << ")";
	return os;
}

size_t L1BusOnWafer::hash() const
{
	size_t hash = 0;
	boost::hash_combine(hash, m_hicann);
	boost::hash_combine(hash, m_horizontal);
	boost::hash_combine(hash, m_id);
	return hash;
}

size_t hash_value(L1BusOnWafer const& bus)
{
	return bus.hash();
}

template <typename Archiver>
void L1BusOnWafer::serialize(Archiver& ar, unsigned int const /*version*/)
{
	using namespace boost::serialization;
	ar & make_nvp("hicann", m_hicann)
	   & make_nvp("horizontal", m_horizontal)
	   & make_nvp("id", m_id);
}

} // namespace routing
} // namespace marocco

BOOST_CLASS_EXPORT_IMPLEMENT(::marocco::routing::L1BusOnWafer)

#include "boost/serialization/serialization_helper.tcc"
EXPLICIT_INSTANTIATE_BOOST_SERIALIZE(::marocco::routing::L1BusOnWafer)
