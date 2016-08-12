#pragma once

#include "marocco/Result.h"
#include "marocco/config.h"
#include "marocco/graph.h"

namespace marocco {

class Algorithm
{
public:
	typedef BaseResult result_type;

	Algorithm(graph_t const& graph,
	          sthal::Wafer& hw,
			  resource_manager_t& mgr);
	virtual ~Algorithm();

	Algorithm(Algorithm const&) = default;
	Algorithm& operator= (Algorithm const&) = default;

protected:
	graph_t const& getGraph() const;

	sthal::Wafer&       getHardware();
	sthal::Wafer const& getHardware() const;

	resource_manager_t&       getManager();
	resource_manager_t const& getManager() const;

private:
	graph_t const&       mGraph;
	sthal::Wafer&   mHW;
	resource_manager_t&  mMgr;
};

} // namespace marocco
