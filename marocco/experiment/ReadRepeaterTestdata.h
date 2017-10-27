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
	ReadRepeaterTestdata(results::Marocco const& results);

	void configure(sthal::Wafer& hardware);
	bool check(sthal::Wafer& hardware, ::HMF::HICANN::BkgRegularISI bkg_gen_isi);

private:
	results::CollectExpectedAddressesByRepeaters m_visitor;
	sthal::ReadRepeaterTestdataConfigurator m_configurator;
}; // ReadRepeaterTestdata

} // namespace experiment
} // namespace marocco
