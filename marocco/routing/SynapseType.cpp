#include "marocco/routing/SynapseType.h"
#include <stdexcept>

namespace marocco {
namespace routing {

SynapseType toSynapseType(Projection::synapse_type const& s)
{
	if (s == "excitatory") {
		return SynapseType::excitatory;
	} else if (s == "inhibitory") {
		return SynapseType::inhibitory;
	} else {
		try {
			// targets "0", "1", "2", ... in multi time constant neurons
			int i = std::stoi(s);
			if (i >= 0)
				return SynapseType(i);
			else
				throw std::runtime_error("Synapse type can not be negative");
		} catch (...) {
			throw std::runtime_error(
				"Invalid synapse type, possible values: "
				"'excitatory', 'inhibitory' and any unsigned integer");
		}
	}
}


std::ostream& operator<<(std::ostream& os, SynapseType const& t)
{
	if (int(t) < -3)
		throw std::runtime_error("unknown switch case");
	switch (t) {
		case SynapseType::None:
			os << "None";
			break;
		case SynapseType::excitatory:
			os << "excitatory";
			break;
		case SynapseType::inhibitory:
			os << "inhibitory";
			break;
		default:
			os << int(t);
			break;
	}
	return os;
}

} // namespace routing
} // namespace marocco
