#pragma once

#if !defined(NDEBUG)
	#if !defined(BOOST_MULTI_INDEX_ENABLE_INVARIANT_CHECKING)
		#define BOOST_MULTI_INDEX_ENABLE_INVARIANT_CHECKING
	#endif
	#if !defined(BOOST_MULTI_INDEX_ENABLE_SAFE_MODE)
		#define BOOST_MULTI_INDEX_ENABLE_SAFE_MODE
	#endif
#endif

#include "marocco/config.h"
#include "pymarocco/PyMarocco.h"

extern bool hw_present;

namespace control {

/*! \class abstract placement interface
 */
class Control
{
protected:
	typedef marocco::hardware_system_t   hardware_type;
	typedef marocco::resource_manager_t  resource_manager_t;

	typedef pymarocco::PyMarocco::Backend Backend;
	typedef pymarocco::PyMarocco::HICANNCfg HICANNCfg;

public:
	Control(hardware_type& hw,
			resource_manager_t const& rmgr,
			pymarocco::PyMarocco& pym);

	// configuration interface
	void run(const double duration_in_s /*seconds*/);

protected:
	void logChip(HMF::Coordinate::HICANNGlobal const& h) const;
	void runExperiment(std::unique_ptr<sthal::ExperimentRunner>&) const;

	void runExperimentAndRecordMembrane(std::unique_ptr<sthal::ExperimentRunner>& runner,
	                                    size_t hicann_global_enum,
	                                    size_t analog_enum,
	                                    double record_in_s) const;

	void pickRunner(std::unique_ptr<sthal::ExperimentRunner>&,
					std::unique_ptr<sthal::HardwareDatabase>&, double dur_in_s) const;

	sthal::HICANNConfigurator* pickHICANNConfigurator() const;

	void dumpWafer(HMF::Coordinate::Wafer const& wafer) const;

	hardware_type& mHW;

	resource_manager_t const& mMgr;

	pymarocco::PyMarocco& mPyMarocco;

};

} // control
