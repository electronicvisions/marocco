#pragma once

#include <set>
#include <map>
#include <utility>

#include "marocco/BioGraph.h"
#include "marocco/config.h"
#include "marocco/results/Marocco.h"
#include "pymarocco/PyMarocco.h"

namespace marocco {

class Mapper
{
public:
	typedef sthal::Wafer hardware_type;

	Mapper(
		hardware_type& hw,
		resource_manager_t& mgr,
		resource_fpga_manager_t& fpga_mgr,
		boost::shared_ptr<pymarocco::PyMarocco> const& pymarocco,
		boost::shared_ptr<results::Marocco> const& results);

	void run(euter::ObjectStore const& pynn);

	hardware_type&       getHardware();
	hardware_type const& getHardware() const;

	pymarocco::MappingStats&       getStats();
	pymarocco::MappingStats const& getStats() const;

	BioGraph const& bio_graph() const;

	boost::shared_ptr<results::Marocco> results();
	boost::shared_ptr<results::Marocco const> results() const;

private:
	BioGraph mBioGraph;

	// resource manager
	resource_manager_t& mMgr;
	resource_fpga_manager_t& mFPGAMgr;

	// and a references to the hw
	hardware_type& mHW;

	boost::shared_ptr<pymarocco::PyMarocco> mPyMarocco;

	boost::shared_ptr<results::Marocco> m_results;
};

} // namespace marocco
