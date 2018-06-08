#pragma once

#include <set>
#include <map>
#include <utility>
#include <memory>

// ESTER MPI header
#include "mpi/config.h"

#include "marocco/graph.h"
#include "marocco/config.h"


// FIXME: due to hacky reverse mapping
#include "marocco/placement/Placement.h"
#include "pymarocco/PyMarocco.h"

namespace marocco {

class Mapper
{
public:
	/// pyNN graph type
	typedef graph_t                       graph_type;

	/// hardware type
	typedef hardware_system_t             hardware_type;

	typedef MPI::Intracomm                comm_type;

	Mapper(hardware_type& hw,
		   resource_manager_t& mgr,
		   comm_type const& /*comm*/,
		   boost::shared_ptr<pymarocco::PyMarocco> const& p = {});
	virtual ~Mapper() {}

	void run(ObjectStore const& pynn);

	graph_type&       getNeuralNetwork();
	graph_type const& getNeuralNetwork() const;

	comm_type&       getComm();
	comm_type const& getComm() const;

	hardware_type&       getHardware();
	hardware_type const& getHardware() const;

	pymarocco::MappingStats&       getStats();
	pymarocco::MappingStats const& getStats() const;

	// TODO: hacky reverse mapping for preliminary reverse channel
	std::shared_ptr<placement::Placement::rev_map_t> getRevMapping() const;

private:
	// the actual pyNN graph,
	graph_type      mGraph;

	// resource manager
	resource_manager_t& mMgr;

	// and a references to the hw
	hardware_type& mHW;

	// communicator of participating mapping processes
	comm_type mComm;

	std::shared_ptr<placement::Placement::rev_map_t> mRevMapping;
	boost::shared_ptr<pymarocco::PyMarocco> mPyMarocco;
};

} // namespace marocco
