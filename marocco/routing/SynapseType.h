#pragma once

#include <iostream>

#include "pywrap/compat/macros.hpp"
#include "marocco/graph.h"

namespace marocco {
namespace routing {

/// synapse type enum.
/// enumeration of synapse types (synaptic targets).
/// Possible values:
///  SynapseType::None
///  SynapseType::excitatory
///  SynapseType::inhibitory
///  SynapseType(0) -> target "0" of multi conductance neurons
///  SynapseType(1) -> target "1" of multi conductance neurons
///  SynapseType(2) -> target "2" of multi conductance neurons
///  etc.
PYPP_TYPED_CLASS_ENUM(SynapseType, int){None = -3, excitatory = -2, inhibitory = -1};

#if !defined(PYPLUSPLUS)
/// convert the target of a PyNN projection aka the synapse type to synapse
/// type enum.
SynapseType toSynapseType(Projection::synapse_type const& s);

std::ostream& operator<<(std::ostream& os, SynapseType const& t);
#endif

} // namespace routing
} // namespace marocco
