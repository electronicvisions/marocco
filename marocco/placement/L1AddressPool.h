#pragma once

#include <deque>

#include "hal/HICANN/L1Address.h"

#include "pymarocco/PyMarocco.h"

namespace marocco {
namespace placement {

/**
 * @brief Provides a pool of available L1 addresses.
 * Single addresses can be removed, marking them as no longer available.
 * @note As four addresses (\c 0bXX0001) are used to implement synaptic weight "0" and
 *       address 0 is used for phase locking of repeaters, not all possible values of
 *       HMF::HICANN::L1Address are present in this pool.
 */
class L1AddressPool
{
public:
	typedef HMF::HICANN::L1Address address_type;
	typedef std::deque<address_type> pool_type;

	L1AddressPool();

	/**
	 * @brief Returns the maximum capacity of the pool.
	 */
	static constexpr size_t capacity()
	{
		return HMF::HICANN::L1Address::size - 1 - 4;
	}

	/**
	 * @brief Removes the highest available address.
	 */
	address_type pop_back();

	/**
	 * @brief Removes the lowest available address.
	 */
	address_type pop_front();

	/**
	 * @brief Removes the next available address, alternating between low and high.
	 */
	address_type pop_alternating();

	/**
	 * @brief Removes an address according to the specified strategy.
	 */
	address_type pop(pymarocco::PyMarocco::L1AddressAssignment const& strategy);


	/**
	 * @brief Returns the number of remaining addresses.
	 */
	size_t size() const;

	/**
	 * @brief Checks whether the pool does not contain any addresses.
	 */
	bool empty() const;

private:
	pool_type m_addresses;
	bool m_alternating_pop_front_next;
}; // L1AddressPool

} // namespace placement
} // namespace marocco
