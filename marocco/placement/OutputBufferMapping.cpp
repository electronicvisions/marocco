#include "marocco/placement/OutputBufferMapping.h"

namespace marocco {
namespace placement {

OutputBufferMapping::OutputBufferMapping() :
	mMode()
{
	for (auto& v : mMode)
		v = Mode::INPUT;
}

bool OutputBufferMapping::any(index_type const& outb) const
{
	return available(outb) != capacity();
}

size_t OutputBufferMapping::available(index_type const& idx) const
{
	return mAddresses[idx].size();
}

void OutputBufferMapping::setMode(index_type const& outb, Mode m)
{
	mMode[outb] = m;
}

OutputBufferMapping::Mode OutputBufferMapping::getMode(index_type const& outb) const
{
	return mMode[outb];
}

L1AddressPool::address_type OutputBufferMapping::popAddress(
	index_type const& ob, pymarocco::PyMarocco::L1AddressAssignment l1_address_assignment)
{
	L1AddressPool& pool = mAddresses[ob];
	if (pool.empty()) {
		throw std::runtime_error("not enough L1 Addresses");
	}

	switch (l1_address_assignment) {
		case pymarocco::PyMarocco::L1AddressAssignment::HighFirst:
			return pool.pop_back();
			break;

		case pymarocco::PyMarocco::L1AddressAssignment::LowFirst:
			return pool.pop_front();
			break;

		case pymarocco::PyMarocco::L1AddressAssignment::Alternate:
			return pool.pop_alternating();
			break;

		default:
			throw std::runtime_error("Unknown L1 address assignment strategy");
	}
}

bool OutputBufferMapping::onlyOutput() const {

	return !std::any_of(mMode.begin(),
						mMode.end(),
						[](Mode m) { return m == Mode::INPUT; });

}

bool OutputBufferMapping::onlyInput() const {

	return !std::any_of(mMode.begin(),
						mMode.end(),
						[](Mode m) { return m == Mode::OUTPUT; });

}

} // namespace placement
} // namespace marocco
