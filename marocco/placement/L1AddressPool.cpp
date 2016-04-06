#include "marocco/placement/L1AddressPool.h"

#include "hal/Coordinate/iter_all.h"
#include "HMF/SynapseDecoderDisablingSynapse.h"

using namespace HMF::Coordinate;
using namespace HMF::HICANN;

namespace marocco {
namespace placement {

namespace {

static L1AddressPool::pool_type valid_addresses()
{
	L1AddressPool::pool_type result;
	// Address 0 is used for phase locking.
	const L1Address addr0(0);
	for (L1Address addr : iter_all<L1Address>()) {
		// The four addresses matching 0bXX0001 are used for analog weight 0.
		if (addr != addr0 && addr.getSynapseDecoderMask() != SynapseDecoderDisablingSynapse)
			result.push_back(addr);
	}
	assert(result.size() == L1AddressPool::capacity());
	return result;
}

} // namespace

L1AddressPool::L1AddressPool() :
	m_addresses(), m_alternating_pop_front_next(true)
{
	static pool_type const addresses = valid_addresses();
	m_addresses = addresses;
}

auto L1AddressPool::pop_back() -> address_type
{
	address_type tmp = m_addresses.back();
	m_addresses.pop_back();
	m_alternating_pop_front_next = true;
	return tmp;
}

auto L1AddressPool::pop_front() -> address_type
{
	address_type tmp = m_addresses.front();
	m_addresses.pop_front();
	m_alternating_pop_front_next = false;
	return tmp;
}

auto L1AddressPool::pop_alternating() -> address_type
{
	if (m_alternating_pop_front_next) {
		return pop_front();
	} else {
		return pop_back();
	}
}

auto L1AddressPool::pop(parameters::L1AddressAssignment::Strategy const& strategy) -> address_type
{
	switch (strategy) {
		case parameters::L1AddressAssignment::Strategy::high_first:
			return pop_back();
		case parameters::L1AddressAssignment::Strategy::low_first:
			return pop_front();
		case parameters::L1AddressAssignment::Strategy::alternating:
			return pop_alternating();
		default:
			throw std::runtime_error("unknown L1 address assignment strategy");
	}
}

size_t L1AddressPool::size() const
{
	return m_addresses.size();
}

bool L1AddressPool::empty() const
{
	return m_addresses.empty();
}

} // namespace placement
} // namespace marocco
