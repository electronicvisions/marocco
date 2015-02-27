#pragma once

#include "pywrap/compat/macros.hpp"
#include <boost/shared_ptr.hpp>
#include <ostream>


// forward declaration
struct SynapseDynamics;

namespace marocco {
namespace routing {

PYPP_CLASS_ENUM(STPMode){off, depression, facilitation};

#if !defined(PYPLUSPLUS)
std::ostream& operator<<(std::ostream& os, STPMode m);
#endif

#if !defined(PYPLUSPLUS)
/// extracts the STP mode from the Projection's synapse dynmics object
STPMode toSTPMode(boost::shared_ptr<const SynapseDynamics> const& dynamics);
#endif


} // namespace routing
} // namespace marocco
