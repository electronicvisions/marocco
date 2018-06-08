#pragma once

#include <tuple>
#include <array>
#include <vector>
#include <algorithm>

#include "pymarocco/PyMarocco.h"
#include "marocco/assignment/AddressMapping.h"
#include "marocco/placement/AddressPool.h"
#include "hal/Coordinate/HMFGeometry.h"

namespace marocco {
namespace placement {

/** Book keeping of modes (input/output) and assigned addresses of output buffers.
 */
class OutputBufferMapping
{
public:
	typedef HMF::Coordinate::OutputBufferOnHICANN index;
	typedef assignment::AddressMapping            assign;
	typedef std::vector<assign>                   list;

	enum : size_t { CAPACITY = AddressPool::CAPACITY };

	OutputBufferMapping();

	/// address mappings for output buffer `idx`
	list const& at(index const& idx) const;

	/// address mappings for output buffer `idx`
	list const& operator[] (index const& idx) const;

	/// assign addresses on output buffer `idx`
	void insert(index const& idx, assign const & a);

	/// true if there is at least one assignment on output buffer `outb`
	bool any(index const& outb);

	/// true if there is at least one assignment on at least one output buffer
	bool any() const;

	/// number of available neuron addresses on output buffer `idx`
	size_t available(index const& idx);

	/// total number of available neuron addresses on all output buffers
	size_t available() const;

	/// available modes
	enum Mode {
		INPUT, //<! OutputBuffer can receive input from L2
		OUTPUT //<! Corresponding DNCMerger forwards spikes to L2
	};

	/// set mode `m` on output buffer `outb`
	void setMode(index const& outb, Mode m);

	/// get mode of output buffer `outb`
	Mode getMode(index const& outb) const;

	/// true if no neurons are placed on output buffer `outb`
	bool empty(index const& outb) const;

	/// pop `N` available addresses from the address pool of output buffer `ob`
	/// in the chosen order
	AddressPool::pool_type popAddresses(index const& ob, size_t N,
	          pymarocco::PyMarocco::L1AddressAssignment l1_address_assignment =
						  pymarocco::PyMarocco::L1AddressAssignment::HighFirst);

	/// check if all output buffers are set to OUTPUT
	bool onlyOutput() const;

	/// check if all output buffers are set to INPUT
	bool onlyInput() const;

private:
	/// address mappings and number of free addresses of output buffers
	std::array<std::pair<list, size_t /*free*/>, index::end> mMapping;

	/// modes of output buffers, has to default to Mode::INPUT
	std::array<Mode, index::end> mMode;

	/// address pools of output buffers
	std::array<AddressPool, index::end> mAddresses;

	template<typename Archive>
	void serialize(Archive& ar, unsigned int const /*version*/)
	{
		using boost::serialization::make_nvp;
		ar & make_nvp("output_buffer_mapping", mMapping)
		   & make_nvp("mode", mMode);
	}
};

} // namespace placement
} // namespace marocco
