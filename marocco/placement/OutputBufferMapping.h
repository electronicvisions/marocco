#pragma once

#include "hal/Coordinate/L1.h"
#include "hal/Coordinate/typed_array.h"

#include "marocco/placement/L1AddressPool.h"
#include "pymarocco/PyMarocco.h"

namespace marocco {
namespace placement {

/** Book keeping of modes (input/output) and assigned addresses of output buffers.
 */
class OutputBufferMapping
{
public:
	typedef HMF::Coordinate::DNCMergerOnHICANN index_type;

	static constexpr size_t capacity()
	{
		return L1AddressPool::capacity();
	}

	OutputBufferMapping();

	/**
	 * @brief Checks whether there are any assignments for the specified merger.
	 * @note Both external and hardware neurons are taken into account.
	 */
	bool any(index_type const& outb) const;

	/**
	 * @brief Returns the number of remaining neuron addresses for the specified merger.
	 */
	size_t available(index_type const& idx) const;

	/// available modes
	enum Mode {
		INPUT, //<! OutputBuffer can receive input from L2
		OUTPUT //<! Corresponding DNCMerger forwards spikes to L2
	};

	/// set mode `m` on output buffer `outb`
	void setMode(index_type const& outb, Mode m);

	/// get mode of output buffer `outb`
	Mode getMode(index_type const& outb) const;

	/// pop next available address from the address pool of output buffer `ob`
	/// in the chosen order
	L1AddressPool::address_type popAddress(
	    index_type const& ob,
	    pymarocco::PyMarocco::L1AddressAssignment l1_address_assignment =
	        pymarocco::PyMarocco::L1AddressAssignment::HighFirst);

	/// check if all output buffers are set to OUTPUT
	bool onlyOutput() const;

	/// check if all output buffers are set to INPUT
	bool onlyInput() const;

private:
	/// modes of output buffers, has to default to Mode::INPUT
	HMF::Coordinate::typed_array<Mode, index_type> mMode;

	/// address pools of output buffers
	HMF::Coordinate::typed_array<L1AddressPool, index_type> mAddresses;
};

} // namespace placement
} // namespace marocco
