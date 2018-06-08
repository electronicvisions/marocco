#pragma once

#include <tbb/mutex.h>
#include "marocco/config.h"
#include "marocco/util.h"

#include "marocco/Algorithm.h"
#include "marocco/placement/Result.h"
#include "marocco/routing/Result.h"
#include "marocco/routing/Route.h"
#include "marocco/routing/SynapseLoss.h"
#include "pymarocco/PyMarocco.h"

namespace marocco {

namespace routing {

struct HICANNRouting :
	public Algorithm
{
public:
	typedef hardware_system_t hardware_type;
	typedef chip_type<hardware_type>::type chip_type;

	HICANNRouting(boost::shared_ptr<SynapseLoss> const& sl,
				  pymarocco::PyMarocco const& pymarocco,
				  graph_t const& nnetwork,
				  hardware_type& hw,
				  resource_manager_t& mgr,
				  comm_t comm,
				  routing_graph const& rgraph);
	virtual ~HICANNRouting();

	std::unique_ptr<SynapseRowRoutingResult> run(
		placement::Result const& placement,
		CrossbarRoutingResult const& routes);

private:
	void run(HMF::Coordinate::HICANNGlobal const& hicann,
			 placement::Result const& placement,
			 CrossbarRoutingResult const& routes,
			 SynapseRowRoutingResult& result);

	void configure_hardware(
		routing::SynapseRowRoutingResult const& syndrvrouting);

	void setSynapseSwitch(
		HMF::Coordinate::VLineOnHICANN const& vline,
		HMF::Coordinate::SynapseSwitchRowOnHICANN const& row,
		chip_type& chip);

	void setSynapseDriver(
		DriverResult const& d,
		chip_type& chip);

	template<typename RAIter>
	void connectSynapseDriver(
		HMF::Coordinate::SynapseDriverOnHICANN const& primary,
		RAIter const first,
		RAIter const last,
		chip_type& chip);

	pymarocco::PyMarocco const& mPyMarocco;
	routing_graph const& mRoutingGraph;
	boost::shared_ptr<SynapseLoss> mSynapseLoss;
	tbb::mutex mMutex;
};

} // namespace routing
} // namespace marocco
