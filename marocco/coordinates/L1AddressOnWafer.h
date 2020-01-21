#pragma once

#include <iostream>
#include <boost/operators.hpp>
#include <boost/serialization/export.hpp>

#include "halco/hicann/v2/hicann.h"
#include "halco/hicann/v2/l1.h"
#include "hal/HICANN/L1Address.h"

namespace marocco {

class L1AddressOnWafer : boost::equality_comparable<L1AddressOnWafer>
{
public:
	L1AddressOnWafer();
	L1AddressOnWafer(
	    halco::hicann::v2::DNCMergerOnWafer const& dnc_merger,
	    HMF::HICANN::L1Address const& address);

	halco::hicann::v2::HICANNOnWafer toHICANNOnWafer() const;
	halco::hicann::v2::DNCMergerOnWafer const& toDNCMergerOnWafer() const;
	halco::hicann::v2::DNCMergerOnHICANN toDNCMergerOnHICANN() const;
	HMF::HICANN::L1Address const& toL1Address() const;

	friend bool operator==(L1AddressOnWafer const& lhs, L1AddressOnWafer const& rhs);

private:
	halco::hicann::v2::DNCMergerOnWafer m_dnc_merger;
	HMF::HICANN::L1Address m_address;

	friend class boost::serialization::access;
	template <typename Archiver>
	void serialize(Archiver& ar, const unsigned int /* version */);
}; // L1AddressOnWafer

std::ostream& operator<<(std::ostream& os, L1AddressOnWafer const& address);
bool operator==(L1AddressOnWafer const& lhs, L1AddressOnWafer const& rhs);
size_t hash_value(L1AddressOnWafer const& nrn);

} // namespace marocco

BOOST_CLASS_EXPORT_KEY(::marocco::L1AddressOnWafer)

namespace std {

template <>
struct hash<marocco::L1AddressOnWafer>
{
	size_t operator()(marocco::L1AddressOnWafer const& nrn) const
	{
		return hash_value(nrn);
	}
};

} // namespace std
