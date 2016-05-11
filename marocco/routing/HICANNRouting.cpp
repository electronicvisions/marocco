#include "marocco/routing/HICANNRouting.h"
#include "marocco/Logger.h"
#include "marocco/routing/SynapseLoss.h"
#include "marocco/routing/SynapseRouting.h"
#include "marocco/routing/util.h"
#include "marocco/util.h"

#include <algorithm>
#include <tbb/parallel_for_each.h>
#include <chrono>

using namespace HMF::Coordinate;
using namespace HMF::HICANN;

namespace marocco {
namespace routing {

HICANNRouting::HICANNRouting(
	BioGraph const& bio_graph,
	hardware_system_t& hardware,
	resource_manager_t& resource_manager,
	pymarocco::PyMarocco const& pymarocco,
	placement::results::Placement const& neuron_placement,
	results::L1Routing const& l1_routing,
	boost::shared_ptr<SynapseLoss> const& synapse_loss)
	: m_bio_graph(bio_graph),
	  m_hardware(hardware),
	  m_resource_manager(resource_manager),
	  m_pymarocco(pymarocco),
	  m_neuron_placement(neuron_placement),
	  m_l1_routing(l1_routing),
	  m_synapse_loss(synapse_loss)
{
}

SynapseRoutingResult
HICANNRouting::run()
{
	SynapseRoutingResult result;

	for (HICANNGlobal const& hicann : m_resource_manager.allocated()) {
		run(hicann, result);
	}

	configure_hardware(result);

	return result;
}

void HICANNRouting::run(HMF::Coordinate::HICANNGlobal const& hicann, SynapseRoutingResult& result)
{
	// synapse routing has to be run no matter there are routes ending at this
	// chip or not, because we need the synapse target mapping for param trafo
	SynapseRouting synapse_routing(
		hicann, m_bio_graph, m_hardware, m_resource_manager, m_pymarocco.synapse_routing,
		m_neuron_placement, m_l1_routing, m_synapse_loss);
	synapse_routing.run();

	auto& res = result[hicann];
	res = std::move(synapse_routing.getResult());
}

void HICANNRouting::configure_hardware(routing::SynapseRoutingResult const& synapse_routing)
{
	// get all used process local chips
	for (auto const& hicann: m_resource_manager.allocated())
	{
		auto& chip = m_hardware[hicann];

		if (!synapse_routing.exists(hicann)) {
			MAROCCO_DEBUG("no SynapseRowRouting Result for " << hicann << " this is ok, if there are "
			   << "no neurons placed on this HICANN or routes could be realized to connect to this neurons");
			continue;
		}

		std::vector<DriverResult> const& synrow_routing = synapse_routing.at(hicann).driver_result;

		// for every target synapse row set switches, driver, weights, synaptic inputs.
		for (DriverResult const& d : synrow_routing)
		{
			for (auto const& driver : d.drivers())
			{
				SynapseDriverOnHICANN const& primary = driver.first;

				if (d.from_adjacent()) {
					HICANNGlobal const& adj = (d.line().toSideHorizontal()==left) ? hicann.east() : hicann.west();
					auto& adjacent_chip = m_hardware[adj];
					setSynapseSwitch(d.line(), primary.toSynapseSwitchRowOnHICANN(), adjacent_chip);
				} else {
					// local synapse switch
					setSynapseSwitch(d.line(), primary.toSynapseSwitchRowOnHICANN(), chip);
				}
			}

			// configure SYNAPSE DRIVERS
			setSynapseDriver(d, chip);
		}

	} // chips

} // configure_hardware()

void HICANNRouting::setSynapseSwitch(
	VLineOnHICANN const& vline,
	SynapseSwitchRowOnHICANN const& row,
	chip_type& chip)
{
	chip.synapse_switches.set(vline, row.line(), true);
}

void HICANNRouting::setSynapseDriver(
	DriverResult const& d,
	chip_type& chip)
{
	// configure synapse driver type and MSB
	for (auto const& entry : d.rows())
	{
		SynapseRowOnHICANN const& row = entry.first;
		SynapseRowSource const& source = entry.second;

		auto& driver = chip.synapses[row.toSynapseDriverOnHICANN()];
		RowConfig& config = driver[row.toRowOnSynapseDriver()];

		// make sure only one synaptic input is connected at a time.
		config.set_syn_in(left, false);
		config.set_syn_in(right, false);
		config.set_syn_in(source.synaptic_input(), true);

		/// there are 4 2-bit decoders per Synapse drivers. Two per synapse row: one
		/// for odd and one for even denmem ids. To keep things simple, we set both
		/// to the same LSB.
		for (auto const& side : { top, bottom })
		{
			config.set_decoder(side, source.prefix(side));
		}
	}

	// configure L1 topology
	for (auto const& entry : d.drivers())
	{
		SynapseDriverOnHICANN const& primary = entry.first;
		std::vector<SynapseDriverOnHICANN> const& list = entry.second;

		if (list.empty()) {
			throw std::runtime_error("empty synapse driver list");
		}

		/// sort synapse driver from top most to bottom most.
		std::vector<SynapseDriverOnHICANN> sorted(list);
		if (!std::is_sorted(sorted.begin(), sorted.end())) {
			MAROCCO_DEBUG("unsorted synapse driver list");
			std::sort(sorted.begin(), sorted.end());
		}

		MAROCCO_INFO("Connecting " << sorted.size() << " synapse driver(s) from "
		                           << sorted.front() << " to " << sorted.back() << " on "
		                           << chip.index());

		if (primary.isTop()) {
			// start with top most synapse driver, its topin bit should not be set.
			connectSynapseDriver(primary, sorted.begin(), sorted.end(), chip);
		} else {
			// start with bottom most synapse driver (note the rbegin()), its topin
			// bit should not be set.
			connectSynapseDriver(primary, sorted.rbegin(), sorted.rend(), chip);
		}
	}

	// configure other parameters
	for (auto const& entry : d.drivers())
	{
		for (auto const& drv : entry.second)
		{
			auto& driver = chip.synapses[drv];

			STPMode const& stp = d.stp_settings().at(drv);
			switch (stp) {
				case STPMode::off:
					driver.disable_stp();
					break;
				case STPMode::depression:
					driver.set_std();
					break;
				case STPMode::facilitation:
					driver.set_stf();
					break;
			}
		}
	}
}

template<typename RAIter>
void HICANNRouting::connectSynapseDriver(
	SynapseDriverOnHICANN const& primary,
	RAIter const first,
	RAIter const last,
	chip_type& chip)
{
	if (first>=last) {
		throw std::runtime_error("invalid SynapseDriver range");
	}

	// first set all driver to either mirror or listen
	for (auto it=first; it<last; ++it)
	{
		auto& drv = chip.synapses[*it];
		if (it == first) {
			drv.set_listen();
		} else {
			drv.set_mirror();
		}
	}

	// finally set primary
	auto& drv = chip.synapses[primary];
	if (*first==primary) {
		drv.set_l1();
	} else {
		drv.set_l1_mirror();
	}
}

} // namespace routing
} // namespace marocco
