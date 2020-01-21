#pragma once

#include "sthal/Wafer.h"
#include "marocco/routing/results/SynapseRouting.h"

namespace marocco {
namespace routing {

class SynapseRoutingConfigurator
{
public:
	SynapseRoutingConfigurator(sthal::Wafer& hardware);

	void run(
	    halco::hicann::v2::HICANNOnWafer const& hicann,
	    results::SynapseRouting::HICANN const& result);

private:
	/**
	 * @brief Set switch connecting a vertical bus to a synapse driver.
	 * @param hicann HICANN of the synapse driver.
	 * @param driver Driver to connect to.
	 * @param vline Vertical bus, may lie on adjacent HICANN.
	 */
	void set_synapse_switch(
	    halco::hicann::v2::HICANNOnWafer const& hicann,
	    halco::hicann::v2::SynapseDriverOnHICANN const& driver,
		halco::hicann::v2::VLineOnHICANN const& vline);

	/**
	 * @brief Connect adjacent synapse drivers.
	 */
	void connect_drivers(
		halco::hicann::v2::HICANNOnWafer const& hicann,
		results::ConnectedSynapseDrivers const& connected_drivers);

	sthal::Wafer& m_hardware;
}; // SynapseRoutingConfigurator

} // namespace routing
} // namespace marocco
