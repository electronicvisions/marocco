#include "marocco/Algorithm.h"

namespace marocco {

Algorithm::Algorithm(
	graph_t const& graph,
	hardware_system_t& hw,
	resource_manager_t& mgr,
	comm_t comm) :
		mGraph(graph),
		mHW(hw),
		mMgr(mgr),
		mComm(comm)
{}

Algorithm::~Algorithm() {}

graph_t const&
Algorithm::getGraph() const
{
	return mGraph;
}

hardware_system_t&
Algorithm::getHardware()
{
	return mHW;
}
hardware_system_t const&
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

comm_t&
Algorithm::getComm()
{
	return mComm;
}
comm_t const&
Algorithm::getComm() const
{
	return mComm;
}

} // namespace marocco
