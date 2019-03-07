#pragma once

#include <set>

#include "hal/Coordinate/HMFGeometry.h"
#include "marocco/graph.h"
#include "marocco/placement/parameters/L1AddressAssignment.h"
namespace marocco {
namespace placement {
namespace results {
class Placement;
} // namespace results
} // namespace placement
namespace resource {
template <class T>
class Manager;
typedef Manager<HMF::Coordinate::HICANNGlobal> HICANNManager;
} // namespace resource
} // namespace marocco


namespace marocco {
namespace placement {

/**
 * @brief this class is a functor providing information whether hardware constrains are violated
 * for a given placement. Different merger tree configurations are queried.
 *
 */
class ConstrainMergers
{
public:
	/**
	 * @brief construct a ConstrainMerger, used to query merger tree validity for a given placement
	 *
	 * @param [in] res_mgr : used to query constraints
	 * @param [in] placement : used to query neuron placement
	 * @param [in] bio_graph : used to find targets
	 * @param [in] strategy : used to simulate the correct L1AddressAssignment strategy
	 */
	ConstrainMergers(
	    resource::HICANNManager const& res_mgr,
	    results::Placement const& placement,
	    graph_t const& bio_graph,
	    parameters::L1AddressAssignment::Strategy const strategy);

	/**
	 * @brief returns true if the constraints are met
	 *
	 * @param [in] dnc_merger : the DNC to test if it is valid
	 * @param [in] adjacent_nbs : the neuron blocks that are merged together
	 * @param [in] hicann : used to query the correct hicann for placement data
	 * @return bool : true if constrains are met, false if violated
	 */
	bool operator()(
	    HMF::Coordinate::DNCMergerOnHICANN const& dnc_merger,
	    std::set<HMF::Coordinate::NeuronBlockOnHICANN> const& adjacent_nbs,
	    HMF::Coordinate::HICANNOnWafer const& hicann) const;


private:
	resource::HICANNManager const& m_res_mgr;
	results::Placement const& m_placement;
	graph_t const& m_bio_graph;
	parameters::L1AddressAssignment::Strategy const m_strategy;
};

} // namespace placement
} // namespace marocco
