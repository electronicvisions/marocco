#include <numeric>

#include "marocco/routing/SynapseRoutingConfigurator.h"

#include "hal/Coordinate/iter_all.h"

#include "marocco/Logger.h"

using namespace HMF::Coordinate;

namespace marocco {
namespace routing {

SynapseRoutingConfigurator::SynapseRoutingConfigurator(sthal::Wafer& hardware)
	: m_hardware(hardware)
{
}

void SynapseRoutingConfigurator::run(
	HMF::Coordinate::HICANNOnWafer const& hicann,
	results::SynapseRouting::HICANN const& result)
{

	std::vector<size_t> n_connected_drivers;

	for (auto const& item : result.synapse_switches()) {
		auto const& vline = item.source();
		auto const& connected_drivers = item.connected_drivers();
		n_connected_drivers.push_back(connected_drivers.size());
		auto const& primary_driver = connected_drivers.primary_driver();

		set_synapse_switch(hicann, primary_driver, vline);
		connect_drivers(hicann, connected_drivers);

		auto& chip = m_hardware[hicann];
		for (auto const& driver : connected_drivers.drivers()) {
			auto& driver_config = chip.synapses[driver];
			for (auto const row_on_driver : iter_all<RowOnSynapseDriver>()) {
				auto const row = SynapseRowOnHICANN(driver, row_on_driver);
				auto& row_config = driver_config[row_on_driver];

				if (!result.has(row)) {
					continue;
				}

				// make sure only one synaptic input is connected at a time.
				row_config.set_syn_in(left, false);
				row_config.set_syn_in(right, false);
				row_config.set_syn_in(SideHorizontal(result[row].synaptic_input()), true);

				// “decoder[0] is for left (even) synapses”
				row_config.set_decoder(top, result[row].address(Parity::even));
				row_config.set_decoder(bottom, result[row].address(Parity::odd));
			}

			if (!result.has(driver)) {
				continue;
			}

			switch (result[driver].stp_mode()) {
				case STPMode::off:
					driver_config.disable_stp();
					break;
				case STPMode::depression:
					driver_config.set_std();
					break;
				case STPMode::facilitation:
					driver_config.set_stf();
					break;
			}
		}
	}

	MAROCCO_DEBUG(
	    "Number of connected drivers on "
	    << hicann << ": "
	    << std::accumulate(n_connected_drivers.begin(), n_connected_drivers.end(), 0.0) /
	           n_connected_drivers.size());

	if (!n_connected_drivers.empty()) {
		MAROCCO_DEBUG(
		    "Minimum number of connected drivers on "
		    << hicann << ": "
		    << *std::min_element(n_connected_drivers.begin(), n_connected_drivers.end()));
	}

	if (!n_connected_drivers.empty()) {
		MAROCCO_DEBUG(
		    "Maximum number of connected drivers on "
		    << hicann << ": "
		    << *std::max_element(n_connected_drivers.begin(), n_connected_drivers.end()));
	}
}

void SynapseRoutingConfigurator::set_synapse_switch(
	HMF::Coordinate::HICANNOnWafer const& hicann,
	HMF::Coordinate::SynapseDriverOnHICANN const& driver,
	HMF::Coordinate::VLineOnHICANN const& vline)
{
	auto hicann_ = hicann;

	// Input from adjacent chip
	if (vline.toSideHorizontal() != driver.toSideHorizontal()) {
		hicann_ = vline.isLeft() ? hicann.east() : hicann.west();
	}

	m_hardware[hicann_].synapse_switches.set(
		vline, driver.toSynapseSwitchRowOnHICANN().line(), true);
}

void SynapseRoutingConfigurator::connect_drivers(
	HMF::Coordinate::HICANNOnWafer const& hicann, results::ConnectedSynapseDrivers const& connected_drivers)
{
	auto const& drivers = connected_drivers.drivers();
	if (drivers.empty()) {
		return;
	}

	MAROCCO_TRACE(
		"Connecting " << drivers.size() << " synapse driver(s) from "
		<< *drivers.begin() << " to " << *drivers.rbegin() << " on " << hicann);

	auto const& primary = connected_drivers.primary_driver();
	auto end_piece = primary.isTop() ? *drivers.begin() : *drivers.rbegin();

	// Set all drivers except for end piece to mirror mode.
	// On the upper (lower) half of the HICANN, analog propagation of the L1 events between adjacent
	// drivers is implemented via a `topin` (`bottomin`, because of mirroring) bit.  Thus all except
	// the uppermost (lowermost) driver (“end piece”) have to be set to `mirror`.
	// The `end_piece` driver is set to `listen`, as it should not have a connection to the next neighbor.
	auto& chip = m_hardware[hicann];
	for (auto const& driver : drivers) {
		chip.synapses[driver].set_mirror();
	}
	// This reverses the effect of `set_mirror()`.
	chip.synapses[end_piece].set_listen();

	// The driver receiving the L1 input needs an additional flag.
	if (primary == end_piece) {
		chip.synapses[primary].set_l1();
	} else {
		chip.synapses[primary].set_l1_mirror();
	}
}

} // namespace routing
} // namespace marocco
