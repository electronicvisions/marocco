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
	 * @param results         	Mapping results.
	 * @param n_locking_retest	Maximum number of times locking is redone in case there
	 *                          are unlocked repeaters.
	 *
	 */
	ReadRepeaterTestdata(results::Marocco const& results, size_t n_locking_retest);

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

private:
	results::CollectExpectedAddressesByRepeaters m_visitor;
	sthal::ReadRepeaterTestdataConfigurator m_configurator;
	results::Marocco const& m_results;
	size_t const m_n_locking_retest; // number of times relocking should be tried
}; // ReadRepeaterTestdata

} // namespace experiment
} // namespace marocco
