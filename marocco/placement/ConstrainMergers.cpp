#include "marocco/placement/ConstrainMergers.h"


#include "marocco/Logger.h"
#include "marocco/placement/internal/L1AddressPool.h"
#include "marocco/placement/parameters/L1AddressAssignment.h"
#include "marocco/placement/results/Placement.h"
#include "marocco/resource/Manager.h"
#include "marocco/routing/SynapseDriverRequirementPerSource.h"

using namespace HMF::Coordinate;

namespace marocco {
namespace placement {

ConstrainMergers::ConstrainMergers(
    resource::HICANNManager const& res_mgr,
    results::Placement const& placement,
    graph_t const& bio_graph,
    parameters::L1AddressAssignment::Strategy const strategy) :
    m_res_mgr(res_mgr),
    m_placement(placement),
    m_bio_graph(bio_graph),
    m_strategy(strategy)
{}

bool ConstrainMergers::operator()(
    HMF::Coordinate::DNCMergerOnHICANN const& dnc_merger,
    std::set<HMF::Coordinate::NeuronBlockOnHICANN> const& adjacent_nbs,
    HMF::Coordinate::HICANNOnWafer const& hicann) const
{
	// simulate L1Address assigment for this merger configuration and return if it would violate HW
	// constrains.
	internal::Result iResult;
	results::Placement l_placement = m_placement;
	DNCMergerOnWafer dnc_on_w(dnc_merger, hicann);

	// Assign L1 addresses for simulation purposes for the DriverChainLength calculation.
	// Code is equals in parts marocco/placement/Placemnt.cpp:129 ff
	auto& address_assignment = iResult.address_assignment[hicann];
	for (auto const nb : adjacent_nbs) {
		NeuronBlockOnWafer const neuron_block(nb, hicann);

		auto neurons_on_nb = l_placement.find(neuron_block);

		if (neurons_on_nb.empty()) {
			// Connected DNC merger can be used for DNC input.
			continue;
		}
		// set this SPL1 merger to output
		// check that merger is either unused or already set to output
		assert(address_assignment.mode(dnc_on_w) != internal::L1AddressAssignment::Mode::input);
		address_assignment.set_mode(dnc_on_w, internal::L1AddressAssignment::Mode::output);
		auto& pool = address_assignment.available_addresses(dnc_on_w);

		for (auto const& item : neurons_on_nb) {
			auto const address = pool.pop(m_strategy);
			auto const& logical_neuron = item.logical_neuron();
			l_placement.set_address(logical_neuron, L1AddressOnWafer(dnc_on_w, address));
		}
	}


	routing::SynapseDriverRequirementPerSource drv_req(m_bio_graph, l_placement);
	// check if the driver requirements for this dnc can be met.

	return drv_req.drivers_possible(dnc_on_w, m_res_mgr);
}

} // namespace placement
} // namespace marocco
