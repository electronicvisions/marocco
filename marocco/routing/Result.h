#pragma once

#include <vector>
#include <string>
#include <iosfwd>

#include <hal/Coordinate/HMFGeometry.h>

#include "marocco/config.h"
#include "marocco/PartialResult.h"
#ifndef PYPLUSPLUS
#include "marocco/ResultInterface.h"
#endif // PYPLUSPLUS
#include "marocco/routing/LocalRoute.h"
#include "marocco/routing/SynapseRowSource.h"
#include "marocco/routing/DriverAssignment.h"

namespace marocco {
namespace routing {

// never forget: synaspses in neuro simulators (and hardware) are local to
// post-neurons. Therfore, the whole way from synapse to, synapsedriver to
// merger tree needs to be traversable from post to pre to e.g. look up analog
// parameters for synapse driver etc.


struct CrossbarRoutingTypes
{
	typedef hardware_system_t hardware_type;
	typedef std::vector<LocalRoute> result_type;
};

class CrossbarRoutingResult :
	public PartialResult<CrossbarRoutingTypes, HMF::Coordinate::HICANNGlobal, 0>
{};

typedef CrossbarRoutingResult route_mapping_t;
typedef CrossbarRoutingResult global_routing_t;

struct SynapseRowRoutingTypes
{
	typedef hardware_system_t hardware_type;

	// this type maps a synapse driver to a single route and some addresses
	typedef std::vector<DriverResult> result_type;
};

/// class storing the the synapse routing results for all HICANNs
class SynapseRowRoutingResult :
	public PartialResult<SynapseRowRoutingTypes, HMF::Coordinate::HICANNGlobal, 1>
{
public:
	std::ostream& operator<< (std::ostream& o) const;
};

typedef SynapseRowRoutingResult synapse_driver_mapping_t;
typedef SynapseRowRoutingResult detailed_routing_t;


#ifndef PYPLUSPLUS
// combine Results
class Result :
	public ResultInterface<CrossbarRoutingResult, SynapseRowRoutingResult>
{};
#endif // PYPLUSPLUS

} // namespace routing
} // namespace marocco
