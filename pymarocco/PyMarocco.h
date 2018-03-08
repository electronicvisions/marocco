#pragma once

#include <string>
#include <boost/serialization/access.hpp>
#include <boost/serialization/export.hpp>

#include "euter/metadata.h"
#include "pywrap/compat/macros.hpp"

#include "pymarocco/MappingStats.h"
#include "pymarocco/Defects.h"
#include "pymarocco/ParamTrafo.h"

#include "marocco/experiment/parameters/Experiment.h"
#include "marocco/placement/parameters/InputPlacement.h"
#include "marocco/placement/parameters/L1AddressAssignment.h"
#include "marocco/placement/parameters/ManualPlacement.h"
#include "marocco/placement/parameters/MergerRouting.h"
#include "marocco/placement/parameters/NeuronPlacement.h"
#include "marocco/routing/parameters/L1Routing.h"
#include "marocco/routing/parameters/SynapseRouting.h"

#include "sthal/ESSConfig.h"

namespace pymarocco {

/// The high-level interface to the maroccco framework
///
/// allows to choose e.g. algorithm parameters or specify the path to the calibration data.
///
/// usage in python:
/// \code{.py}
///  import pyhmf as pynn
///  from pymarocco import PyMarocco
///
///  marocco = PyMarocco()
///  marocco.neuron_placement.default_neuron_size(4)
///  marocco.calib_backend = PyMarocco.XML # load calibration from xml files
///  marocco.calib_path = "/wang/data/calibration/wafer_0"
///
///  pynn.setup(marocco=marocco)
///
/// \endcode
class PyMarocco : public DerivedMetaData<PyMarocco>
{
public:

	PYPP_CLASS_ENUM(Backend) {
		None,
		ESS,
		Hardware
	};

	PYPP_CLASS_ENUM(CalibBackend) {
		XML,
		Binary,
		Default
	};

	PYPP_CLASS_ENUM(HICANNCfg) {
		HICANNConfigurator,
		HICANNv4Configurator,
		DontProgramFloatingGatesHICANNConfigurator,
		NoFGConfigurator,
		NoResetNoFGConfigurator,
		ParallelHICANNv4Configurator,
		ParallelHICANNNoFGConfigurator,
		ParallelHICANNNoResetNoFGConfigurator,
		OnlyNeuronNoResetNoFGConfigurator
	};

	PYPP_CLASS_ENUM(Verification) {
		Verify,
		Skip,
		VerifyButIgnore
	};


	PYPP_CLASS_ENUM(CheckL1Locking) {
		Check,
		SkipCheck,
		CheckButIgnore
	};

	/// choose emulation backend
	/// one of [None, ESS, Hardware]
	/// default: None
	Backend backend;

	/// choose backend for calibration data
	/// one of [XML, Binary, Default]
	/// default: Binary
	CalibBackend calib_backend;

	/// path to directory containing calibration data as xml-files
	/// The calibration is looked up in xml files named w0-h84.xml, w0-h276.xml, etc.
	/// Throws if the environment variable MAROCCO_CALIB_PATH is set
	/// and this string is non-empty.
	/// default: /wang/data/calibration/brainscales/default
	std::string calib_path;

#if !defined(PYPLUSPLUS)
	MappingStats&       getStats();
#endif
	MappingStats const& getStats() const;
	void setStats(MappingStats const& s);

	/// returns meta data name of pymarocco: 'marocco', to be used in pyhmf.setup()
	virtual std::string name() const;

	static boost::shared_ptr<PyMarocco> create();

	marocco::placement::parameters::InputPlacement input_placement;
	marocco::placement::parameters::ManualPlacement manual_placement;
	marocco::placement::parameters::MergerRouting merger_routing;
	marocco::placement::parameters::NeuronPlacement neuron_placement;
	marocco::placement::parameters::L1AddressAssignment l1_address_assignment;
	marocco::routing::parameters::L1Routing l1_routing;
	marocco::routing::parameters::SynapseRouting synapse_routing;
	marocco::experiment::parameters::Experiment experiment;

	/// python property to access mapping stats
	MappingStats stats;

	/// defect information
	Defects defects;

	/// parameters for the transformation of neuron and synapse parameters
	ParamTrafo param_trafo;

	/** Default wafer to fall back to when no manual placement or defects were specified.
	 *  @note: This will go away in the long run. Do \b NOT use this anywhere but rely
	 *  on \c HICANNManager.wafers() instead.
	 *  Defaults to \c 33.
	 */
	HMF::Coordinate::Wafer default_wafer;

	/// path to graphviz bio graph output file. No bio graph output if unset
	std::string bio_graph;

	/**
	 * @brief Do not run mapping step but load results from previous run.
	 * @see \c wafer_cfg and \c persist which must be set.
	 * Defaults to \c false.
	 */
	bool skip_mapping;

	/// choose mode for chip configuration verification
	/// one of [Verify, Skip, VerifyButIgnore]
	/// Verify: read back the written chip configuration and compare against wanted
	/// Skip: no read back of configuration
	/// VerfifyButIgnore: read back the written chip configuration and compare against wanted but
	/// continue in the case of disagreement
	/// default: Verify
	Verification verification;

	/**
	* @brief: check l1 locking of horizontal and vertical repeaters
	* one of [Check, SkipCheck, CheckButIgnore]
	* Check: check if all repeaters accessible from all l1 routes are locked and abort if not
	* SkipCheck: do not check for locked repeaters
	* CheckButIgnore: perform checks but do not abort in case of errors
	*/
	CheckL1Locking checkl1locking;

	/**
	 * @brief Path to file to which is used to store wafer configuration.
	 * @note Existing files will be overwritten.
	 * If this parameter is empty, no file will be written.
	 */
	std::string wafer_cfg;

	/**
	 * @brief Path to file which is used to store mapping results.
	 * @note Existing files will be overwritten.
	 * If this parameter is empty, no file will be written.
	 */
	std::string persist;

	/// inter spike interval (isi) of background generators used for locking in HICANN clk cycles
	/// hence the frequency of the background generators is given by: pll_freq/bkg_gen_isi
	/// default: 125
	uint32_t bkg_gen_isi;

	/// HICANN PLL clock frequency in Hz, e.g. 100e6, 150e6, 200e6
	/// default: 125e6,
	double pll_freq;

	/// choose HICANN configurator
	/// default: ParallelHICANNv4Configurator
	HICANNCfg hicann_configurator;

	/// ESS config
	sthal::ESSConfig ess_config;

	/// specifies the directory name where all temporary files are stored during ESS simulation.
	/// If not specified a temp directory below "/tmp" is created which will be removed at the end of the program.
	/// Instead, a user specified directory is not deleted.
	std::string ess_temp_directory;

	/// if true, the run continues although synapses were lost
	/// default: false
	bool continue_despite_synapse_loss;

private:
	PyMarocco();

	friend class boost::serialization::access;
	template<typename Archive>
	void serialize(Archive& ar, unsigned int const /* version */);
};

} // pymarocco

BOOST_CLASS_EXPORT_KEY(::pymarocco::PyMarocco)
