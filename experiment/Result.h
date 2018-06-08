#pragma once

#include "marocco/config.h"
#include "marocco/placement/Placement.h"
#include "marocco/util.h"
#include "pymarocco/PyMarocco.h"

namespace experiment {

class ReadResults
{
public:
	typedef marocco::hardware_system_t hardware_type;
	typedef marocco::resource_manager_t resource_manager_t;
	typedef MPI::Intracomm comm_t;
	typedef marocco::placement::Placement::rev_map_t rev_map_t;

	ReadResults(
			pymarocco::PyMarocco const& pymarocco,
			hardware_type const& hw,
			resource_manager_t const& mgr);

	void run(ObjectStore& objectstore, rev_map_t const& rev) const;

private:
	/// translate hardware spike times in seconds back to the biological time domain in ms.
	double translate(double hw_cycle) const;
	void insertRandomSpikes(ObjectStore& objectstore) const;

	pymarocco::PyMarocco const& mPyMarocco;
	hardware_type const& mHW;
	resource_manager_t const& mMgr;
};

} // exp
