#pragma once

#include <vector>
#include <deque>

#include <boost/serialization/serialization.hpp>
#include <boost/serialization/vector.hpp>

#include "marocco/assignment/PopulationSlice.h"

// AddressMapping.h is parsed by gccxml because of roqt
// -> cannot simply include AddressPool.h which uses c++11 features
//#include "marocco/placement/AddressPool.h"

#include "hal/HICANN/L1Address.h"

namespace marocco {
namespace assignment {

/**
 * Instances of `AddressMapping` are used in the OutputBufferMapping result to
 * map biological neuron/sources with their assigned L1Addresses to
 * OutputBuffers.
 */
class AddressMapping
{
public:
	typedef assignment::PopulationSlice assignment_type;

	// define address type and pool again, has to
	// match definitions in AddressPool
	typedef HMF::HICANN::L1Address address_type;
	typedef std::deque<address_type> pool_type;

	AddressMapping(assignment_type const& assign,
	               pool_type addr);

	bool operator==(AddressMapping const& rhs) const;

	/// returns the number of neurons/sources in this AdressMapping
	size_t size() const;

	assignment_type const& bio() const;
#ifndef PYPLUSPLUS
	assignment_type&       bio();
#endif // PYPLUSPLUS

	std::vector<address_type> const& addresses() const;
#ifndef PYPLUSPLUS
	std::vector<address_type>&       addresses();
#endif // PYPLUSPLUS

	// check for consistency
	void check() const;

private:
	friend class boost::serialization::access;
	template<typename Archiver>
	void serialize(Archiver& ar, unsigned int const /*version*/);

	/// Default constructor necessary for serialization (but private)
	AddressMapping();

	/// references part of the population contributing this output
	assignment_type                 mAssignment;

	/// vector of addresses sending on this l1 bus
	std::vector<address_type>       mAddresses;
};



template<typename Archiver>
void AddressMapping::serialize(Archiver& ar, unsigned int const /*version*/)
{
	using namespace boost::serialization;
	ar & make_nvp("assignment", mAssignment)
	   & make_nvp("addresses", mAddresses);
}

} // namespace assignment
} // namespace marocco
