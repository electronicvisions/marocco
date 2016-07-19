#pragma once

#include <iosfwd>
#include <string>

#include "pywrap/compat/macros.hpp"

namespace marocco {

/**
 * @brief Synapse types (synaptic targets) of a projection.
 * Apart from excitatory and inhibitory synapses, the targets "0", "1", … of multi
 * conductance neurons are represented by \c SynapseType(0), \c SynapseType(1), ….
 */
PYPP_TYPED_CLASS_ENUM(SynapseType, int){none = -3, excitatory = -2, inhibitory = -1};

/**
 * @brief Convert the target type string of a PyNN projection to a synapse type.
 */
SynapseType toSynapseType(std::string const& str);

std::ostream& operator<<(std::ostream& os, SynapseType const& synapse_type);

} // namespace marocco
