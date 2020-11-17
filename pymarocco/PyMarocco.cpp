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

PyMarocco::PyMarocco()
    : backend(Backend::Without),
      calib_backend(CalibBackend::Binary),
      calib_path("/wang/data/calibration/brainscales/default"),
      default_wafer(33),
      skip_mapping(false),
      verification(Verification::Verify),
      checkl1locking(CheckL1Locking::Check),
      n_locking_retest(5),
      n_locking_recheck_after_ok(5),
      time_between_rechecks(0),
      bkg_gen_isi(125),
      pll_freq(125e6),
      hicann_configurator(new sthal::ParallelHICANNv4SmartConfigurator()),
      continue_despite_synapse_loss(false),
      ensure_analog_trigger(EnsureAnalogTrigger::Ensure),
      scrutinize_mapping(ScrutinizeMapping::Scrutinize)
{}

boost::shared_ptr<PyMarocco> PyMarocco::create()
{
	return boost::shared_ptr<PyMarocco>(new PyMarocco);
}

template<typename Archive>
void PyMarocco::serialize(Archive& ar, unsigned int const version)
{
	using namespace boost::serialization;
	// clang-format off
	ar & make_nvp("input_placement", input_placement)
	   & make_nvp("manual_placement", manual_placement)
	   & make_nvp("merger_routing", merger_routing)
	   & make_nvp("neuron_placement", neuron_placement)
	   & make_nvp("l1_address_assignment", l1_address_assignment)
	   & make_nvp("l1_routing", l1_routing)
	   & make_nvp("synapse_routing", synapse_routing)
	   & make_nvp("experiment", experiment)
	   & make_nvp("stats", stats)
	   & make_nvp("defects", defects)
	   & make_nvp("param_trafo", param_trafo)
	   & make_nvp("default_wafer", default_wafer)
	   & make_nvp("persist", persist)
	   & make_nvp("wafer_cfg", wafer_cfg)
	   & make_nvp("skip_mapping", skip_mapping)
	   & make_nvp("verification", verification)
	   & make_nvp("checkl1locking", checkl1locking)
	   & make_nvp("n_locking_retest", n_locking_retest)
	   & make_nvp("n_locking_recheck_after_ok", n_locking_recheck_after_ok)
	   & make_nvp("time_between_rechecks", time_between_rechecks)
	   & make_nvp("bkg_gen_isi", bkg_gen_isi)
	   & make_nvp("pll_freq", pll_freq)
	   & make_nvp("hicann_configurator", hicann_configurator)
	   & make_nvp("ess_config", ess_config)
	   & make_nvp("ess_temp_directory", ess_temp_directory)
	   & make_nvp("continue_despite_synapse_loss", continue_despite_synapse_loss);
	if (version == 0) {
		ensure_analog_trigger = EnsureAnalogTrigger::EnsureButIgnore;
	}
	if (version > 0) {
		ar & make_nvp("ensure_analog_trigger", ensure_analog_trigger);
	}
	if (version < 2) {
		scrutinize_mapping = ScrutinizeMapping::Scrutinize;
	}
	if (version > 1) {
		ar & make_nvp("scrutinize_mapping", scrutinize_mapping);
	}
	// clang-format on
}

} // pymarocco

BOOST_CLASS_EXPORT_IMPLEMENT(::pymarocco::PyMarocco)
BOOST_CLASS_VERSION(::pymarocco::PyMarocco, 2)

#include "boost/serialization/serialization_helper.tcc"
EXPLICIT_INSTANTIATE_BOOST_SERIALIZE(::pymarocco::PyMarocco)
