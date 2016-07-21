#pragma once

#include <set>
#include <map>
#include <utility>
#include <memory>

#include "marocco/BioGraph.h"
#include "marocco/config.h"
#include "marocco/results/Marocco.h"
#include "pymarocco/PyMarocco.h"

namespace marocco {

class Mapper
{
public:
	/// hardware type
	typedef hardware_system_t             hardware_type;

	Mapper(hardware_type& hw,
		   resource_manager_t& mgr,
		   boost::shared_ptr<pymarocco::PyMarocco> const& p = {});
	virtual ~Mapper() {}

	void run(ObjectStore const& pynn);

	hardware_type&       getHardware();
	hardware_type const& getHardware() const;

	pymarocco::MappingStats&       getStats();
	pymarocco::MappingStats const& getStats() const;

	BioGraph const& bio_graph() const;

	results::Marocco& results();
	results::Marocco const& results() const;

private:
	BioGraph mBioGraph;

	// resource manager
	resource_manager_t& mMgr;

	// and a references to the hw
	hardware_type& mHW;

	boost::shared_ptr<pymarocco::PyMarocco> mPyMarocco;

	std::unique_ptr<results::Marocco> m_results;
};

} // namespace marocco
