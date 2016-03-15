#pragma once

#include <vector>
#include <string>
#include <iosfwd>

#include <hal/Coordinate/HMFGeometry.h>

#include "marocco/config.h"
#include "marocco/Result.h"
#include "marocco/AssociativeResult.h"
#include "marocco/routing/LocalRoute.h"
#include "marocco/routing/SynapseRowSource.h"
#include "marocco/routing/DriverAssignment.h"
#ifndef PYPLUSPLUS
#include "marocco/routing/SynapseTargetMapping.h"
#endif // !PYPLUSPLUS

namespace marocco {
namespace routing {

// never forget: synaspses in neuro simulators (and hardware) are local to
// post-neurons. Therfore, the whole way from synapse to, synapsedriver to
// merger tree needs to be traversable from post to pre to e.g. look up analog
// parameters for synapse driver etc.

class CrossbarRoutingResult
	: public AssociativeResult<HMF::Coordinate::HICANNGlobal, std::vector<LocalRoute> >
{};

typedef CrossbarRoutingResult route_mapping_t;
typedef CrossbarRoutingResult global_routing_t;


/// Combined result for SynapseRouting
struct DriverResultAndSynapseTargetMapping
{
	// this type maps a synapse driver to a single route and some addresses
	std::vector<DriverResult> driver_result;

#ifndef PYPLUSPLUS
	// this type maps a the biological synapse targets to the synaptic input of hardwar neurons
	SynapseTargetMapping synapse_target_mapping;
#endif // !PYPLUSPLUS

	friend class boost::serialization::access;
	template <typename Archiver>
	void serialize(Archiver& ar, unsigned int const /*version*/);
};

/// class storing the synapse routing results for all HICANNs
class SynapseRoutingResult
	: public AssociativeResult<HMF::Coordinate::HICANNGlobal, DriverResultAndSynapseTargetMapping>
{
public:
	std::ostream& operator<<(std::ostream& o) const;
};

typedef SynapseRoutingResult synapse_driver_mapping_t;
typedef SynapseRoutingResult detailed_routing_t;

#ifndef PYPLUSPLUS
struct Result : public BaseResult {
	CrossbarRoutingResult crossbar_routing;
	SynapseRoutingResult synapse_routing;
};

template <typename Archiver>
void DriverResultAndSynapseTargetMapping::serialize(Archiver& ar, unsigned int const /*version*/)
{
	using namespace boost::serialization;
	ar & make_nvp("driver_result", driver_result)
	   & make_nvp("synapse_target_mapping", synapse_target_mapping);
}
#endif // !PYPLUSPLUS

} // namespace routing
} // namespace marocco
