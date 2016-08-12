#include "marocco/Algorithm.h"

namespace marocco {

Algorithm::Algorithm(
	graph_t const& graph,
	sthal::Wafer& hw,
	resource_manager_t& mgr) :
		mGraph(graph),
		mHW(hw),
		mMgr(mgr)
{}

Algorithm::~Algorithm() {}

graph_t const&
Algorithm::getGraph() const
{
	return mGraph;
}

sthal::Wafer&
Algorithm::getHardware()
{
	return mHW;
}
sthal::Wafer const&
Algorithm::getHardware() const
{
	return mHW;
}

resource_manager_t&
Algorithm::getManager()
{
	return mMgr;
}
resource_manager_t const&
Algorithm::getManager() const
{
	return mMgr;
}

} // namespace marocco
