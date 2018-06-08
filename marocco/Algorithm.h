#pragma once

#include "marocco/BaseResult.h"
#include "marocco/config.h"
#include "marocco/graph.h"

namespace marocco {

class Algorithm
{
public:
	typedef BaseResult result_type;

	Algorithm(graph_t const& graph,
			  hardware_system_t& hw,
			  resource_manager_t& mgr,
			  comm_t comm);
	virtual ~Algorithm();

	Algorithm(Algorithm const&) = default;
	Algorithm& operator= (Algorithm const&) = default;

protected:
	graph_t const& getGraph() const;

	hardware_system_t&       getHardware();
	hardware_system_t const& getHardware() const;

	resource_manager_t&       getManager();
	resource_manager_t const& getManager() const;

	comm_t&       getComm();
	comm_t const& getComm() const;

private:
	graph_t const&       mGraph;
	hardware_system_t&   mHW;
	resource_manager_t&  mMgr;
	comm_t               mComm;
};

} // namespace marocco
