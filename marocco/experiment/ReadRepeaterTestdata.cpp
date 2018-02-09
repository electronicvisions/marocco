#include "marocco/experiment/ReadRepeaterTestdata.h"

#include "sthal/Neurons.h"
#include "hal/HICANN/L1Address.h"

#include "marocco/Logger.h"

using namespace HMF::Coordinate;

namespace marocco {
namespace experiment {

ReadRepeaterTestdata::ReadRepeaterTestdata(results::Marocco const& results)
{
	m_visitor.apply(results);
}

void ReadRepeaterTestdata::configure(sthal::Wafer& hardware)
{
	for (auto hicann : hardware.getAllocatedHicannCoordinates()) {
		MAROCCO_DEBUG(hicann);
		auto hicann_global = HICANNGlobal(hicann, hardware.index());
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
						m_configurator.passive_hrepeater_map[hicann_global].insert(*r);
					}
				}

				if (auto const* r = boost::get<VRepeaterOnHICANN>(&kv.first)) {
					MAROCCO_DEBUG("\t setting input for " << *r);
					m_configurator.passive_vrepeater_map[hicann_global].insert(*r);
				}

				for (auto addr : kv.second) {
					MAROCCO_DEBUG("\t\t" << addr);
				}
			}
		}
	}
}

bool ReadRepeaterTestdata::check(sthal::Wafer& hardware,
                                 ::HMF::HICANN::BkgRegularISI bkg_gen_isi) {
	hardware.configure(m_configurator);
	MAROCCO_INFO(m_configurator);
	auto result =
	    m_configurator.analyze_all({::HMF::HICANN::L1Address(0)}, {bkg_gen_isi.value()});
	auto n_good = result.first;
	auto n_total = result.second;
	if (n_good != n_total) {
		MAROCCO_WARN("Only " << n_good << " of " << n_total << " checked repeater(s) are locked!");
		return false;
	}

	if (n_good != 0) {
		MAROCCO_INFO("All " << n_good << " checked repeater(s) are locked.");
	} else {
		MAROCCO_WARN("No repeaters found to be checked for locking!");
	}

	return true;
}

} // namespace experiment
} // namespace marocco
