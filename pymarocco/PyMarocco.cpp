#include "pymarocco/PyMarocco.h"

namespace pymarocco {

MappingStats& PyMarocco::getStats()
{
	return stats;
}

MappingStats const& PyMarocco::getStats() const
{
	return stats;
}

void PyMarocco::setStats(MappingStats const& s)
{
	stats = s;
}

std::string
PyMarocco::name() const
{
	return "marocco";
}

PyMarocco::PyMarocco() :
	backend(Backend::None),
	calib_backend(CalibBackend::Default),
	calib_path(""),
	bkg_gen_isi(500),
	only_bkg_visible(false),
	pll_freq(100e6),
	hicann_configurator(HICANNCfg::HICANNConfigurator),
	speedup(10000.),
	experiment_time_offset(20e-6),
	l1_address_assignment(L1AddressAssignment::HighFirst)
{}

boost::shared_ptr<PyMarocco> PyMarocco::create()
{
	return boost::shared_ptr<PyMarocco>(new PyMarocco);
}

} // pymarocco
