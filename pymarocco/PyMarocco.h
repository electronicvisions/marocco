#pragma once

#include <string>
#include <boost/serialization/nvp.hpp>
#include <pywrap/compat/macros.hpp>
#include <euter/metadata.h>

#include "pymarocco/MappingStats.h"
#include "pymarocco/Placement.h"
#include "pymarocco/InputPlacement.h"
#include "pymarocco/Defects.h"
#include "pymarocco/Routing.h"
#include "pymarocco/RoutingPriority.h"
#include "pymarocco/ParamTrafo.h"

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
///  marocco.placement.setDefaultNeuronSize(4)
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
		DB,
		XML,
		Default
	};

	PYPP_CLASS_ENUM(HICANNCfg) {
		HICANNConfigurator,
		DontProgramFloatingGatesHICANNConfigurator
	};

	PYPP_CLASS_ENUM(L1AddressAssignment) {
		HighFirst,
		LowFirst,
		Alternate // high, low, high, low
	};

	/// choose emulation backend
	/// one of [None, ESS, Hardware]
	/// default: None
	Backend backend;

	/// choose backend for calibration data
	/// one of [DB, XML, Default]
	/// default: Default
	CalibBackend calib_backend;

	/// path to directory containing calibration data as xml-files
	/// The calibration is looked up in xml files named w0-h84.xml, w0-h276.xml, etc.
	std::string calib_path;

#if !defined(PYPLUSPLUS)
	MappingStats&       getStats();
#endif
	MappingStats const& getStats() const;
	void setStats(MappingStats const& s);

	/// returns meta data name of pymarocco: 'marocco', to be used in pyhmf.setup()
	virtual std::string name() const;

	static boost::shared_ptr<PyMarocco> create();

	/// python property to access mapping stats
	MappingStats stats;

	/// parameters and constraints for placement algorithm (manual placement, ...)
	Placement placement;

	/// parameters  for input placement algorithm (e.g. consider bandwidth limits)
	InputPlacement input_placement;

	/// defect information
	Defects defects;

	/// routing priorities for projections
	RoutingPriority routing_priority;

	/// parameters for routing algorithms
	Routing routing;

	/// parameters for the transformation of neuron and synapse parameters
	ParamTrafo param_trafo;

	/// path to output file for routing visualizer (RoQt). No roqt output if unset
	std::string roqt;

	/** Default wafer to fall back to when no manual placement or defects were specified.
	 *  @note: This will go away in the long run. Do \b NOT use this anywhere but rely
	 *  on \c HICANNManager.wafers() instead.
	 */
	HMF::Coordinate::Wafer default_wafer;

	/// path to graphviz bio graph output file. No bio graph output if unset
	std::string bio_graph;

	/// path to file to which wafer configuration is dumped. No output if unset
	std::string wafer_cfg;

	/// path to file to which recorded membrane potential is written. No output if unset
	/// only one neuron can be recorded at a time, calling record_v()
	/// also the hicann enum and analog recorder enum have to be adjusted manually
	std::string membrane;

	/// global enum id of hicann on which membrane is recorded
	size_t hicann_enum;

	/// enum id of analog output channel for membrane recording
	size_t analog_enum;

	/// inter spike interval (isi) of background generators used for locking in HICANN clk cycles
	/// hence the frequency of the background generators is given by: pll_freq/bkg_gen_isi
	/// default: 500
	uint32_t bkg_gen_isi;

	/// enable debug feature: configure network as usual but decode only address 0 (background) events
	/// default: false
	bool only_bkg_visible;

	/// HICANN PLL clock frequency in Hz, e.g. 100e6, 150e6, 200e6
	/// default: 100e6,
	double pll_freq;

	/// choose HICANN configurator
	/// one of: [HICANNConfigurator, DontProgramFloatingGatesHICANNConfigurator]
	HICANNCfg hicann_configurator;


	/// speedup of emulation on hardware compared to biological experiment definition
	/// default: 10000.
	double speedup;

	/// time offset in seconds after which the real experiment on the hardware starts.
	/// this experiment offset is required for the Layer 1 repeater to lock their PLLs.
	/// default: 20e-6;
	///
	/// furthermore, a minimal delay around 500 nS is required so that external pulses
	/// can be injected into the L1 routing at time 0. of the real experiment
	/// Hence, when using the ESS as backend, a value of 5e-7 is sufficient.
	double experiment_time_offset;

	/// order in which L1 addresses are assigned
	/// default: HighFirst
	L1AddressAssignment l1_address_assignment;

	/// specifies the directory name where all temporary files are stored during ESS simulation.
	/// If not specified a temp directory below "/tmp" is created which will be removed at the end of the program.
	/// Instead, a user specified directory is not deleted.
	std::string ess_temp_directory;

private:
	PyMarocco();

	friend class boost::serialization::access;
	template<typename Archive>
	void serialize(Archive& ar, unsigned int const)
	{
		using boost::serialization::make_nvp;
		ar & make_nvp("backend", backend)
		   & make_nvp("calib_backend", calib_backend)
		   & make_nvp("calib_path", calib_path)
		   & make_nvp("stats", stats)
		   & make_nvp("placement", placement)
		   & make_nvp("input_placement", input_placement)
		   & make_nvp("defects", defects)
		   & make_nvp("routing_priority", routing_priority)
		   & make_nvp("routing", routing)
		   & make_nvp("param_trafo", param_trafo)
		   & make_nvp("roqt", roqt)
		   & make_nvp("bio_graph", bio_graph)
		   & make_nvp("wafer_cfg", wafer_cfg)
		   & make_nvp("membrane", membrane)
		   & make_nvp("hicann_enum", hicann_enum)
		   & make_nvp("analog_enum", analog_enum)
		   & make_nvp("bkg_gen_isi", bkg_gen_isi)
		   & make_nvp("only_bkg_visible", only_bkg_visible)
		   & make_nvp("pll_freq", pll_freq)
		   & make_nvp("hicann_configurator", hicann_configurator)
		   & make_nvp("speedup", speedup)
		   & make_nvp("experiment_time_offset", experiment_time_offset)
		   & make_nvp("l1_address_assignment", l1_address_assignment)
		   & make_nvp("ess_temp_directory", ess_temp_directory);
	}
};

} // pymarocco
