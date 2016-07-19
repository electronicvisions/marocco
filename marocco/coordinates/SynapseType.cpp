#include "marocco/coordinates/SynapseType.h"

#include <iostream>
#include <stdexcept>

namespace marocco {

SynapseType toSynapseType(std::string const& str)
{
	if (str == "excitatory") {
		return SynapseType::excitatory;
	} else if (str == "inhibitory") {
		return SynapseType::inhibitory;
	}

	int val;
	// targets "0", "1", "2", ... in multi time constant neurons
	try {
		val = std::stoi(str);
	} catch (std::invalid_argument const&) {
		throw std::runtime_error(
			"synapse type has to be \"excitatory\", \"inhibitory\" "
			"or any unsigned integer");
	}

	if (val < 0) {
		throw std::runtime_error("synapse type cannot be negative");
	}
	return SynapseType(val);
}

std::ostream& operator<<(std::ostream& os, SynapseType const& synapse_type)
{
	switch (synapse_type) {
		case SynapseType::none:
			os << "SynapseType::none";
			return os;
		case SynapseType::excitatory:
			os << "SynapseType::excitatory";
			return os;
		case SynapseType::inhibitory:
			os << "SynapseType::inhibitory";
			return os;
	}

	auto const val = int(synapse_type);
	if (val < 0) {
		throw std::runtime_error("invalid synapse type");
	}
	os << "SynapseType(" << val << ")";
	return os;
}

} // namespace marocco
