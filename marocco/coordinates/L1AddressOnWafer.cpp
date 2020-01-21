#include "marocco/coordinates/L1AddressOnWafer.h"

#include <boost/functional/hash.hpp>
#include <boost/serialization/nvp.hpp>

namespace marocco {

L1AddressOnWafer::L1AddressOnWafer(
    halco::hicann::v2::DNCMergerOnWafer const& dnc_merger,
    HMF::HICANN::L1Address const& address)
    : m_dnc_merger(dnc_merger), m_address(address)
{
}

L1AddressOnWafer::L1AddressOnWafer()
    : m_dnc_merger(), m_address()
{
}

halco::hicann::v2::HICANNOnWafer L1AddressOnWafer::toHICANNOnWafer() const
{
	return m_dnc_merger.toHICANNOnWafer();
}

halco::hicann::v2::DNCMergerOnWafer const& L1AddressOnWafer::toDNCMergerOnWafer() const
{
	return m_dnc_merger;
}

halco::hicann::v2::DNCMergerOnHICANN L1AddressOnWafer::toDNCMergerOnHICANN() const
{
	return m_dnc_merger.toDNCMergerOnHICANN();
}

HMF::HICANN::L1Address const& L1AddressOnWafer::toL1Address() const
{
	return m_address;
}

std::ostream& operator<<(std::ostream& os, L1AddressOnWafer const& address)
{
	os << "L1AddressOnWafer(" << address.toDNCMergerOnWafer() << ", " << address.toL1Address()
	   << ")";
	return os;
}

bool operator==(L1AddressOnWafer const& lhs, L1AddressOnWafer const& rhs)
{
	return (
	    lhs.toDNCMergerOnWafer() == rhs.toDNCMergerOnWafer() &&
	    lhs.toL1Address() == rhs.toL1Address());
}

size_t hash_value(L1AddressOnWafer const& address)
{
	size_t hash = 0;
	boost::hash_combine(hash, address.toDNCMergerOnWafer());
	boost::hash_combine(hash, address.toL1Address());
	return hash;
}

template <typename Archiver>
void L1AddressOnWafer::serialize(Archiver& ar, const unsigned int /* version */)
{
	using namespace boost::serialization;
	// clang-format off
	ar & make_nvp("dnc_merger", m_dnc_merger)
	   & make_nvp("address", m_address);
	// clang-format on
}

} // namespace marocco

BOOST_CLASS_EXPORT_IMPLEMENT(::marocco::L1AddressOnWafer)

#include "boost/serialization/serialization_helper.tcc"
EXPLICIT_INSTANTIATE_BOOST_SERIALIZE(::marocco::L1AddressOnWafer)
