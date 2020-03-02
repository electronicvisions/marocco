#pragma once

#include "hal/HICANNContainer.h"

#include "sthal/Wafer.h"
#include "sthal/ReadRepeaterTestdataConfigurator.h"

#include "marocco/results/CollectExpectedAddressesByRepeaters.h"
#include "marocco/results/Marocco.h"

namespace marocco {
namespace experiment {

class ReadRepeaterTestdata {
public:
	/**
	 * Constructor
	 *
	 * @param results                   Mapping results.
	 * @param n_locking_retest           Maximum number of times locking is redone in case
	 *                                   there are unlocked repeaters.
	 * @param n_locking_retest_after_ok Maximum number of times locking is checked again
	 *                                   after a successful initial lock
	 * @param time_between_rechecks     Time in ms between rechecks after successful locking
	 *
	 */
	ReadRepeaterTestdata(results::Marocco const& results,
	                     size_t n_locking_retest,
	                     size_t n_locking_recheck_after_ok,
	                     size_t time_between_rechecks);

	void configure(sthal::Wafer& hardware);

	/**
	 * Check if individual repeaters are locked. For each repeater block which contains at
	 * least one unlocked repeater locking is redone. The maximal number of relocking is given
	 * during construction.
	 *
	 * @param hardware Wafer which should be checked.
	 * @param bkg_gen_isi Inter spike interval of the background generators.
	 */
	bool check(sthal::Wafer& hardware, ::HMF::HICANN::BkgRegularISI bkg_gen_isi);

	/**
	 * Checks in the same way as check() if the repeaters in the network are locked but does
	 * not redo the locking of the repeaters.
	 *
	 * @param hardware     Wafer which should be checked.
	 * @param bkg_gen_isi  Inter spike interval of the background generators.
	 * @param n_rechecks   Number of times locking is checked.
	 * @param waiting_time Waiting time  in ms after each check of the repeater locking.
	 */
	bool recheck(sthal::Wafer& hardware,
	             ::HMF::HICANN::BkgRegularISI bkg_gen_isi,
	             size_t const n_rechecks,
	             size_t const waiting_time);
private:
	results::CollectExpectedAddressesByRepeaters m_visitor;
	sthal::ReadRepeaterTestdataConfigurator m_configurator;
	results::Marocco const& m_results;
	size_t const m_n_locking_retest; // number of times relocking should be tried
	// number of times locking should be tested after successful locking
	size_t const m_n_locking_recheck_after_ok;
	// waiting time in ms after each check of the repeater locking after an initial successful lock
	size_t const m_time_between_rechecks;
}; // ReadRepeaterTestdata

} // namespace experiment
} // namespace marocco
