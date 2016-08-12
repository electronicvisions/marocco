#pragma once

#include <memory>

#include "marocco/graph.h"
#include "marocco/placement/Result.h"
#include "pymarocco/PyMarocco.h"

namespace marocco {
namespace placement {

class Placement
{
public:
	typedef BaseResult result_type;

	Placement(
	    pymarocco::PyMarocco& pymarocco,
	    graph_t const& graph,
	    sthal::Wafer& hardware,
	    resource_manager_t& resource_manager);

	std::unique_ptr<result_type> run(results::Placement& result);

private:
	graph_t const& m_graph;
	sthal::Wafer& m_hardware;
	resource_manager_t& m_resource_manager;
	pymarocco::PyMarocco& m_pymarocco;
}; // Placement

} // namespace placement
} // namespace marocco
