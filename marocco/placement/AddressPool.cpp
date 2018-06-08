#include "marocco/placement/AddressPool.h"

#include "hal/Coordinate/iter_all.h"
#include "HMF/SynapseDecoderDisablingSynapse.h"

using namespace HMF::Coordinate;
using namespace HMF::HICANN;

namespace marocco {
namespace placement {

AddressPool::AddressPool() :
	mAddr()
{
	static pool_type const _default = init();
	mAddr = _default;
}

AddressPool::address_type AddressPool::pop_back()
{
	address_type tmp = mAddr.back();
	mAddr.pop_back();
	return tmp;
}

AddressPool::address_type AddressPool::pop_front()
{
	address_type tmp = mAddr.front();
	mAddr.pop_front();
	return tmp;
}

size_t AddressPool::size() const
{
	return mAddr.size();
}

AddressPool::pool_type const&
AddressPool::available() const
{
	return mAddr;
}

// return list of available Layer 1 addresses. Address 0 is used for phase locking.
// and four 0bXX0001 are used for analog weight 0.
static std::vector<int> generate_valid_addresses() {
	std::vector<int> rv;
	const L1Address addr0(0);
	for (L1Address addr :  iter_all<L1Address>() ) {
		if ( addr != addr0 && addr.getSynapseDecoderMask() != SynapseDecoderDisablingSynapse )
			rv.push_back(addr.value());
	}
	return rv;
}


AddressPool::pool_type
AddressPool::init() const
{
	static std::vector<int> const addresses = generate_valid_addresses();

	pool_type r(addresses.begin(), addresses.end());

	if (r.size() != CAPACITY) {
		throw std::runtime_error("wrong capacity");
	}
	return r;
}

} // namespace placement
} // namespace marocco
