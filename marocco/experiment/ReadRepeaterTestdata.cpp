#include "marocco/experiment/ReadRepeaterTestdata.h"

#include "sthal/Neurons.h"
#include "sthal/ParallelOnlyRepeaterLockingNoResetNoFGConfigurator.h"
#include "sthal/ParallelHICANNv4SmartConfigurator.h"
#include "hal/HICANN/L1Address.h"

#include "marocco/Logger.h"

using namespace halco::hicann::v2;

namespace marocco {
namespace experiment {

ReadRepeaterTestdata::ReadRepeaterTestdata(results::Marocco const& results, size_t n_locking_retest)
    : m_results(results), m_n_locking_retest(n_locking_retest)
{
	m_visitor.apply(results);
}

void ReadRepeaterTestdata::configure(sthal::Wafer& hardware)
{
	for (auto hicann : hardware.getAllocatedHicannCoordinates()) {
		MAROCCO_DEBUG(hicann);
		for (auto kv : m_visitor.expected_addresses[hicann]) {
			MAROCCO_DEBUG("\t expected " << kv.first);
			for (auto addr : kv.second) {
				MAROCCO_DEBUG("\t\t" << addr);
			}
		}

		for (auto kv : m_visitor.passive_addresses[hicann]) {
			if (m_visitor.expected_addresses[hicann].find(kv.first) ==
				m_visitor.expected_addresses[hicann].end()) {
				MAROCCO_DEBUG("\t passive " << kv.first);

				if (auto const* r = boost::get<HRepeaterOnHICANN>(&kv.first)) {
					// sending repeaters don't have support for test output
					if(!r->isSending()) {
						MAROCCO_DEBUG("\t setting input for " << *r);
						m_configurator.add_passive_hrepeater(HRepeaterOnWafer(*r, hicann));
					}
				}

				if (auto const* r = boost::get<VRepeaterOnHICANN>(&kv.first)) {
					MAROCCO_DEBUG("\t setting input for " << *r);
					m_configurator.add_passive_vrepeater(VRepeaterOnWafer(*r, hicann));
				}

				for (auto addr : kv.second) {
					MAROCCO_DEBUG("\t\t" << addr);
				}
			}
		}
	}
}

bool ReadRepeaterTestdata::check(sthal::Wafer& hardware, ::HMF::HICANN::BkgRegularISI bkg_gen_isi)
{
	bool locked = false;
	size_t n_retest = 0;

	size_t n_good = 0;
	size_t n_bad = 0;
	size_t n_total = 0;

	while (!locked) {

		MAROCCO_DEBUG("Check if all repeaters are locked.");

		hardware.configure(m_configurator);
		MAROCCO_DEBUG(m_configurator);

		auto const bad_good_hr =
		    m_configurator.analyze_hrepeater({::HMF::HICANN::L1Address(0)}, {bkg_gen_isi.value()});
		auto const bad_good_vr =
		    m_configurator.analyze_vrepeater({::HMF::HICANN::L1Address(0)}, {bkg_gen_isi.value()});

		n_bad = bad_good_hr.first.size() + bad_good_vr.first.size();
		n_good = bad_good_hr.second.size() + bad_good_vr.second.size();
		n_total = n_bad + n_good;

		if (n_bad == 0) {
			locked = true;
			break;
		}

		MAROCCO_DEBUG("Only " << n_good << " of " << n_total << " checked repeater(s) are locked!");

		if (n_retest >= m_n_locking_retest){
			break;
		}

		// go through the route and report if repeater is locked
		for (auto const& route_item : m_results.l1_routing) {
			MAROCCO_TRACE("######################################################################");

			auto const& route = route_item.route();

			HICANNOnWafer current_hicann;

			for (auto rs : route.segments()) {
				if (auto const* r = boost::get<HICANNOnWafer>(&rs)) {
					current_hicann = *r;
					MAROCCO_TRACE(current_hicann);
				} else if (auto const* r = boost::get<VLineOnHICANN>(&rs)) {
					auto const vrepeater_on_wafer =
					    VRepeaterOnWafer(r->toVRepeaterOnHICANN(), current_hicann);
					auto it = std::find(
					    bad_good_vr.first.begin(), bad_good_vr.first.end(), vrepeater_on_wafer);
					if (it != bad_good_vr.first.end()) {
						MAROCCO_WARN(rs << " is bad (" << it->toVRepeaterOnHICANN()
						                << " unlocked)");
					} else {
						MAROCCO_TRACE(rs);
					}
				} else if (auto const* r = boost::get<HLineOnHICANN>(&rs)) {
					auto const hrepeater_on_wafer =
					    HRepeaterOnWafer(r->toHRepeaterOnHICANN(), current_hicann);
					auto it = std::find(
					    bad_good_hr.first.begin(), bad_good_hr.first.end(), hrepeater_on_wafer);
					if (it != bad_good_hr.first.end()) {
						MAROCCO_WARN(rs << " is bad (" << it->toHRepeaterOnHICANN()
						                << " unlocked)");
					} else {
						MAROCCO_TRACE(rs);
					}
				} else {
					MAROCCO_TRACE(rs);
				}
			}
		}

		// relock repeaters
		std::set<RepeaterBlockOnWafer> rbs;
		for (auto hr : bad_good_hr.first) {
			rbs.insert(hr.toRepeaterBlockOnWafer());
		}
		for (auto vr : bad_good_vr.first) {
			rbs.insert(vr.toRepeaterBlockOnWafer());
		}
		MAROCCO_DEBUG("Redo locking " << n_retest + 1 << " of " << m_n_locking_retest);
		auto only_repeater_locking_configurator =
			sthal::ParallelOnlyRepeaterLockingNoResetNoFGConfigurator(rbs);
		hardware.configure(only_repeater_locking_configurator);

		++n_retest;
	}

	MAROCCO_DEBUG("Lock synapse drivers");
	sthal::ParallelHICANNv4SmartConfigurator only_synapse_driver_locking_configurator;
	// set configurator up to skip all configuration steps and enable synapse driver
	// locking explicitly
	only_synapse_driver_locking_configurator.set_skip();
	only_synapse_driver_locking_configurator.syn_drv_locking_config_mode =
		sthal::ParallelHICANNv4SmartConfigurator::ConfigMode::Force;
	hardware.configure(only_synapse_driver_locking_configurator);

	if (locked) {
		if (n_good != 0) {
			MAROCCO_INFO("All " << n_good << " checked repeater(s) are locked.");
		} else {
			MAROCCO_WARN("No repeaters found to be checked for locking!");
		}
		return true;
	} else {
		MAROCCO_WARN("After " << m_n_locking_retest <<  " relocking cycles only "
		             << n_good << " of " << n_total << " checked repeater(s) are locked!");
		return false;
	}
}

} // namespace experiment
} // namespace marocco
