#include "test/common.h"
#include "marocco/placement/HICANNPlacement.h"
#include "pymarocco/Placement.h"
#include <memory>
#include <boost/make_shared.hpp>

#include "redman/backend/MockBackend.h"

using namespace std;
using namespace HMF::Coordinate;

namespace marocco {
namespace placement {

TEST(HICANNPlacement, Basic)
{
	auto backend = boost::make_shared<redman::backend::MockBackend>();
	Wafer wafer;

	pymarocco::Placement py_placement;
	graph_t graph;
	hardware_system_t hw;
	resource_manager_t mgr{backend, {wafer}};
	comm_t comm;

	hw[wafer]; // hack: to allocate a wafer;
	HICANNPlacement placement(py_placement, graph, hw, mgr, comm);
	placement.run();
}

} // placement
} // marocco
