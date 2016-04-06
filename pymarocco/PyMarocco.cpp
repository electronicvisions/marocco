#include "pymarocco/PyMarocco.h"

#include <boost/serialization/nvp.hpp>

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
	membrane_translate_to_bio(true),
	bkg_gen_isi(500),
	only_bkg_visible(false),
	pll_freq(100e6),
	hicann_configurator(HICANNCfg::HICANNConfigurator),
	speedup(10000.),
	experiment_time_offset(20e-6)
{}

boost::shared_ptr<PyMarocco> PyMarocco::create()
{
	return boost::shared_ptr<PyMarocco>(new PyMarocco);
}

template<typename Archive>
void PyMarocco::serialize(Archive& ar, unsigned int const /* version */)
{
	using namespace boost::serialization;
	// clang-format off
	ar & make_nvp("input_placement", input_placement)
	   & make_nvp("manual_placement", manual_placement)
	   & make_nvp("merger_routing", merger_routing)
	   & make_nvp("neuron_placement", neuron_placement)
	   & make_nvp("l1_address_assignment", l1_address_assignment)
	   & make_nvp("stats", stats)
	   & make_nvp("defects", defects)
	   & make_nvp("routing_priority", routing_priority)
	   & make_nvp("routing", routing)
	   & make_nvp("param_trafo", param_trafo)
	   & make_nvp("roqt", roqt)
	   & make_nvp("default_wafer", default_wafer)
	   & make_nvp("bio_graph", bio_graph)
	   & make_nvp("wafer_cfg", wafer_cfg)
	   & make_nvp("wafer_cfg_inject", wafer_cfg_inject)
	   & make_nvp("membrane", membrane)
	   & make_nvp("membrane_translate_to_bio", membrane_translate_to_bio)
	   & make_nvp("hicann_enum", hicann_enum)
	   & make_nvp("analog_enum", analog_enum)
	   & make_nvp("bkg_gen_isi", bkg_gen_isi)
	   & make_nvp("only_bkg_visible", only_bkg_visible)
	   & make_nvp("pll_freq", pll_freq)
	   & make_nvp("hicann_configurator", hicann_configurator)
	   & make_nvp("speedup", speedup)
	   & make_nvp("experiment_time_offset", experiment_time_offset)
	   & make_nvp("ess_temp_directory", ess_temp_directory);
	// clang-format on
}

} // pymarocco

BOOST_CLASS_EXPORT_IMPLEMENT(::pymarocco::PyMarocco)

#include "boost/serialization/serialization_helper.tcc"
EXPLICIT_INSTANTIATE_BOOST_SERIALIZE(::pymarocco::PyMarocco)
