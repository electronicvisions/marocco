#pragma once

#include <deque>
#include <vector>
#include "hal/HICANN/L1Address.h"

namespace marocco {
namespace placement {

/**
 * The AddressPool provides a list of available L1Addressess, which can be popped
 * and therefore marked as no longer available. This is primarily useful for the
 * input/neuron mapping to OputputBuffer.
 **/
class AddressPool
{
public:
	typedef HMF::HICANN::L1Address  address_type;
	typedef std::deque<address_type> pool_type;

	enum : size_t { CAPACITY = 59 };

	AddressPool();
	AddressPool(AddressPool const&) = default;
	AddressPool(AddressPool&&) = default;

	/// pops an available address from pool.
	/// high addresses come first
	address_type pop_back();

	/// pops an available address from pool.
	/// low addresses come first
	address_type pop_front();

	/// number of still available L1Addresses.
	size_t size() const;

	pool_type const& available() const;

private:

	/// initializes the address pool
	/// high addresses must be at the end
	pool_type init() const;

	pool_type mAddr;
};

} // namespace placement
} // namespace marocco
