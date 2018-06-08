#include "marocco/assignment/AddressMapping.h"

namespace marocco {
namespace assignment {

AddressMapping::AddressMapping(assignment_type const& assign,
                               pool_type addr) :
	mAssignment(assign),
	mAddresses(addr.begin(), addr.end())
{}

bool AddressMapping::operator==(AddressMapping const& rhs) const
{
	return mAssignment == rhs.mAssignment
		&& addresses().size() == rhs.addresses().size()
		&& std::equal(addresses().begin(),
					  addresses().end(),
					  rhs.addresses().begin());
}

size_t AddressMapping::size() const
{
	assert(mAssignment.size() == addresses().size());
	return mAssignment.size();
}

AddressMapping::assignment_type const&
AddressMapping::bio() const
{
	return mAssignment;
}

AddressMapping::assignment_type&
AddressMapping::bio()
{
	return mAssignment;
}

std::vector<AddressMapping::address_type>&
AddressMapping::addresses()
{
	return mAddresses;
}

std::vector<AddressMapping::address_type> const&
AddressMapping::addresses() const
{
	return mAddresses;
}

void AddressMapping::check() const
{
	if (mAssignment.size() != addresses().size())
		throw std::runtime_error("fatal source error");

	// TODO: add consistency check for addresses. compare with real hardware
	// object
}

AddressMapping::AddressMapping()
{}

} // namespace assignment
} // namespace marocco
