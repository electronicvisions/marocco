#include "marocco/routing/SynapseRouting.h"

#include <cstdlib>
#include <functional>
#include <iterator>
#include <stdexcept>
#include <boost/make_shared.hpp>

#include "calibtic/HMF/SynapseDecoderDisablingSynapse.h"
#include "halco/hicann/v2/synapse.h"
#include "halco/common/iter_all.h"
#include "halco/common/typed_array.h"

#include "marocco/Logger.h"
#include "marocco/routing/Fieres.h"
#include "marocco/routing/HandleSynapseLoss.h"
#include "marocco/routing/SynapseDriverRequirements.h"
#include "marocco/routing/SynapseLoss.h"
#include "marocco/routing/SynapseManager.h"
#include "marocco/routing/internal/SynapseTargetMapping.h"
#include "marocco/routing/util.h"

// NOTE: always use clear vertex and maybe edge lists, rather than vectors,
// because we have a rather dynamically changing graph.
// Resource allocate for e.g. synapse driver will remove edges.

using namespace halco::hicann::v2;
using namespace halco::common;
using namespace HMF::HICANN;

namespace marocco {
namespace routing {

using namespace euter;

template <typename T>
using by_side_type = typed_array<T, SideHorizontal>;

template <typename T>
using by_vline_type = std::unordered_map<VLineOnHICANN, T>;

SynapseRouting::SynapseRouting(
	halco::hicann::v2::HICANNGlobal const& hicann,
	BioGraph const& bio_graph,
	hardware_type& hardware,
	resource_manager_t& resource_manager,
	parameters::SynapseRouting const& parameters,
	placement::results::Placement const& neuron_placement,
	results::L1Routing const& l1_routing,
	boost::shared_ptr<SynapseLoss> const& synapse_loss,
	results::SynapseRouting& result)
	: m_hicann(hicann),
	  m_bio_graph(bio_graph),
	  m_hardware(hardware),
	  m_resource_manager(resource_manager),
	  m_parameters(parameters),
	  m_neuron_placement(neuron_placement),
	  m_l1_routing(l1_routing),
	  m_synapse_loss(synapse_loss),
	  m_result(result)
{
}

void SynapseRouting::run()
{
	auto& chip = m_hardware[m_hicann];

	// firstly set all SynapseDecoders to 0bXX0001 addresses
	invalidateSynapseDecoder();
	tagDefectSynapses();

	// mapping of synapse targets (excitatory, inhibitory) to synaptic inputs of denmems
	results::SynapticInputs& synaptic_inputs = m_result[m_hicann].synaptic_inputs();

	internal::SynapseTargetMapping::simple_mapping(
		m_hicann, m_neuron_placement, m_bio_graph.graph(), synaptic_inputs);
	assert(synaptic_inputs.is_horizontally_symmetrical());
	MAROCCO_DEBUG("SynapticInputs:\n" << synaptic_inputs);

	MAROCCO_INFO("calc synapse driver requirements for hicann " << m_hicann);

	SynapseDriverRequirements drivers_required(m_hicann, m_neuron_placement, synaptic_inputs);

	// Helper to handle synapse loss for whole routes.
	HandleSynapseLoss handle_synapse_loss(
		m_bio_graph, m_neuron_placement, m_l1_routing, m_synapse_loss);

	//  ——— synapse driver requirements ————————————————————————————————————————
	// The number of required synapse drivers are calculated for all routes ending at the
	// current HICANN.  This is done separately for drivers on the left and right side.

	// An incoming L1 route is uniquely identified by its vline and the side the synapse
	// driver is on, as lines from adjacent HICANNs fulfill `vline.side() != side` and can
	// be distinguished from a local line with the same id.

	by_side_type<by_vline_type<std::reference_wrapper<results::L1Routing::route_item_type const> > >
		route_item_by_source;

	typedef std::vector<DriverInterval> IntervalList;
	by_side_type<IntervalList> requested_drivers_per_side;

	by_side_type<SynapseManager::HistMap> synapse_histogram;
	by_side_type<SynapseManager::HistMap> synrow_histogram;

	for (auto const& route_item : m_l1_routing.find_routes_to(m_hicann)) {
		auto const& route = route_item.route();
		VLineOnHICANN const vline = boost::get<VLineOnHICANN>(route.back());
		DNCMergerOnWafer const source_dnc = route_item.source();

		SideHorizontal drv_side = vline.toSideHorizontal();
		if (route.target_hicann() != m_hicann) {
			// Connection from adjacent HICANN.
			drv_side = (drv_side == left) ? right : left;
		}

		auto const needed = drivers_required.calc(
			source_dnc, m_bio_graph.graph(),
			synapse_histogram[drv_side][vline],
			synrow_histogram[drv_side][vline]);

		MAROCCO_TRACE(
			"route " << vline << " requires " << needed.first << " synapse drivers for "
			         << needed.second << " synapses");

		// this can only happen in very rare ocasions, where by chance, no
		// weight has been realized in the projectionView. More likely for lower
		// connection probs.
		if (needed.first == 0u) {
			MAROCCO_WARN(
				"no synapse driver needed for route from " << route_item.source()
				<< " to " << route_item.target());
			handle_synapse_loss(route_item);
			continue;
		}

		requested_drivers_per_side[drv_side].emplace_back(vline, needed.first, needed.second);
		auto res = route_item_by_source[drv_side].insert(std::make_pair(vline, std::cref(route_item)));
		if (!res.second) {
			throw std::runtime_error("insertion error");
		}
	}

	size_t const chain_length = m_parameters.driver_chain_length();
	if (chain_length != SynapseDriverOnQuadrant::size) {
		MAROCCO_DEBUG("using non-maximum synapse driver chain length of " << chain_length);
	}


	std::unordered_map<halco::hicann::v2::NeuronOnHICANN,
					   std::map<SynapseType, SynapseDriverRequirements::SynapseColumnsMap> >
		synapse_type_to_synapse_columns_map =
			drivers_required.get_synapse_type_to_synapse_columns_map();

	for (auto const& drv_side : iter_all<SideHorizontal>()) {
		IntervalList const& requested_drivers =
			requested_drivers_per_side[drv_side];
		if (requested_drivers.empty()) {
			continue;
		}

		// prepare defect list first.
		// Use std set first to prevent duplicates
		std::set<SynapseDriverOnHICANN> defect_set;
		auto const& defects = m_resource_manager.get(m_hicann);
		auto const& drvs = defects->drivers();
		MAROCCO_INFO("Collecting defect synapse drivers");
		for (auto const& drv : drvs->disabled())
		{
			defect_set.insert(drv);
			MAROCCO_TRACE("Marked " << drv << " on " << m_hicann << " as defect/disabled");
		}
		// Also disable drivers of defect synapse arrays
		// Disabling synapses instead would lead to L1 loss
		auto const& arrays = defects->synapsearrays();
		MAROCCO_INFO("Collecting defect synapse arrays and disable related synapse drivers");
		for (auto const& array : arrays->disabled()) {
			for (auto const& drv : iter_all<SynapseDriverOnHICANN>()) {
				if (drv.toSynapseArrayOnHICANN() == array) {
					defect_set.insert(drv);
					MAROCCO_TRACE("Marked " << drv << " on " << m_hicann << " as defect/disabled");
				}
			}
		}

		std::vector<SynapseDriverOnHICANN> defect_list(defect_set.begin(), defect_set.end());

		size_t const n_marked_syndrvs = boost::size(drvs->disabled());
		if (n_marked_syndrvs != 0) {
			MAROCCO_DEBUG("Marked " << n_marked_syndrvs << " synapse driver(s) on "
			                        << m_hicann << " as defect/disabled");
		}

		// prepare defects list of synapse switches
		std::set<SynapseSwitchOnHICANN> defect_synapse_switches;
		auto const& syn_switches = defects->synapseswitches();
		MAROCCO_INFO("Collecting defect synapse switches");
		for (auto const& syn_switch : syn_switches->disabled()) {
			defect_synapse_switches.insert(syn_switch);
			MAROCCO_TRACE("Marked " << syn_switch << " on " << m_hicann << " as defect/disabled");
		}

		size_t const n_marked_syn_switches = boost::size(syn_switches->disabled());
		if (n_marked_syn_switches != 0) {
			MAROCCO_DEBUG(
			    "Marked " << n_marked_syn_switches << " synapse switch(es) on " << m_hicann
			              << " as defect/disabled");
		}

		Fieres fieres(
		    requested_drivers, drv_side, chain_length, defect_list, defect_synapse_switches);
		std::vector<VLineOnHICANN> const rejected = fieres.rejected();
		auto const result = fieres.result();

		{
			size_t used = 0;
			for (auto const& item : result) {
				for (auto const& assignment : item.second) {
					used += assignment.size();
				}
			}
			MAROCCO_INFO(
				"Used " << used << " synapse drivers on " << drv_side << " of " << m_hicann);
		}

		// mark synapses lost beloning to rejected incoming routes
		for (auto const& vline : rejected) {
			auto const& route_item = route_item_by_source[drv_side].at(vline).get();
			MAROCCO_WARN(
				"Could not allocate synapse driver for route from " << route_item.source()
				<< " to " << route_item.target());
			handle_synapse_loss(route_item);
		}


		// manages the SynapseRows assigned to local routes but it
		// differentiates between sources with different MSBs and different
		// synapse types (exc/inh).
		SynapseManager syn_manager(result);
		syn_manager.init(synapse_histogram[drv_side], synrow_histogram[drv_side]);

		for (auto const& entry : result)
		{
			VLineOnHICANN const& vline = entry.first;
			auto const& route_item = route_item_by_source[drv_side].at(vline).get();
			auto const& route = route_item.route();
			DNCMergerOnWafer const route_source_merger = route_item.source();
			HICANNOnWafer const route_source_hicann = route.source_hicann();
			assert(route_item.target() == m_hicann);
			MAROCCO_TRACE(
			    "======================================================================\n"
			    "route from "
			    << route_source_merger << " to " << route.back() << " on "
			    << route.target_hicann());

			SynapseManager::SynapsesOnVLine synapses_on_vline =
				syn_manager.getSynapses(vline, synapse_type_to_synapse_columns_map);
			syn_manager.check(chain_length);

			////////////////////////////////////////
			// S T O R E   D R I V E R   C O N F I G
			////////////////////////////////////////

			for (results::ConnectedSynapseDrivers const& connected_drivers : entry.second) {
				MAROCCO_TRACE("using drivers: " << connected_drivers);
				m_result[m_hicann].add_synapse_switch(vline, connected_drivers);
			}

			// insert synapse row configuration
			auto const& hw_prop_to_syn_row_assignment = syn_manager.get(vline);
			for (auto const& item : hw_prop_to_syn_row_assignment) {
				SynapseManager::SubRows const& rows = item.second;

				if (rows.empty()) {
					continue;
				}

				Side side;
				Parity parity;
				DriverDecoder decoder;
				STPMode stp;
				std::tie(side, parity, decoder, stp) = item.first;
				SynapticInputOnNeuron synaptic_input(side);

				if (m_parameters.only_allow_background_events()) {
					decoder = DriverDecoder(0);
				}

				for (auto const& row : rows) {
					{ // set stp mode
						SynapseDriverOnHICANN const driver = row.toSynapseDriverOnHICANN();
						// if there already exists a config, check for equality
						if (m_result[m_hicann].has(driver)) {
							if (m_result[m_hicann][driver].stp_mode() != stp) {
								throw std::runtime_error(
								    "different stp settings requested on one synapse driver");
							}
						}
						m_result[m_hicann][driver].set_stp_mode(stp);
					}

					{ // set synaptic input and L1 address decoder mask
						if (m_result[m_hicann].has(row)) {
							if (m_result[m_hicann][row].synaptic_input() != synaptic_input) {
								throw std::runtime_error(
								    "different synaptic input side settings "
								    "requested on one synapse row");
							}
						}
						auto& row_config = m_result[m_hicann][row];
						row_config.set_synaptic_input(synaptic_input);
						row_config.set_address(parity, decoder);
					}
				}
			}

			//////////////////////////////////
			// P R O C E S S   S Y N A P S E S
			//////////////////////////////////

			for (auto const& proj_item : m_l1_routing.find_projections(route_item)) {
				auto const edge = m_bio_graph.edge_from_id(proj_item.edge());
				auto const source = boost::source(edge, m_bio_graph.graph());
				auto const target = boost::target(edge, m_bio_graph.graph());


				MAROCCO_TRACE(
				    "----------------------------------------------------------------------\n"
				    "projection "
				    << proj_item.projection() << " from " << *(m_bio_graph.graph()[source])
				    << " to " << *(m_bio_graph.graph()[target]));

				// get synloss proxy object for faster loss counting
				SynapseLossProxy syn_loss_proxy =
					m_synapse_loss->getProxy(edge, route_source_hicann, m_hicann);

				auto const proj_view = boost::make_shared<ProjectionView>(m_bio_graph.graph()[edge]);
				Connector::const_matrix_view_type const bio_weights = proj_view->getWeights();
				SynapseType const syntype_proj = toSynapseType(proj_view->projection()->target());
				STPMode const stp_proj = toSTPMode(proj_view->projection()->dynamics());

				for (auto const& source_item : m_neuron_placement.find(source)) {
					auto const& address = source_item.address();
					// Only process source neuron placements matching current route.
					if (address == boost::none ||
						address->toDNCMergerOnWafer() != route_source_merger) {
						continue;
					}

					if (!proj_view->pre().mask()[source_item.neuron_index()]) {
						continue;
					}

					MAROCCO_TRACE("from " << source_item.bio_neuron() << " with " << *address);

					for (auto const& target_item : m_neuron_placement.find(target)) {
						auto const& neuron_block = target_item.neuron_block();
						// Only process targets on current HICANN.
						if (neuron_block == boost::none ||
							neuron_block->toHICANNOnWafer() != m_hicann) {
							continue;
						}

						if (!proj_view->post().mask()[target_item.neuron_index()]) {
							continue;
						}

						MAROCCO_TRACE(
						    "to " << target_item.bio_neuron() << " at "
						          << target_item.logical_neuron().front());

						size_t const src_neuron_in_proj_view =
							to_relative_index(proj_view->pre().mask(), source_item.neuron_index());
						size_t const trg_neuron_in_proj_view =
							to_relative_index(proj_view->post().mask(), target_item.neuron_index());

						double const weight =
							bio_weights(src_neuron_in_proj_view, trg_neuron_in_proj_view);

						if (std::isnan(weight) || weight <= 0.) {
							continue;
						}

						auto const& logical_neuron = target_item.logical_neuron();
						assert(!logical_neuron.is_external());
						NeuronOnHICANN const target_nrn = logical_neuron.front();

						L1Address const& l1_address = address->toL1Address();
						DriverDecoder const driver_dec = l1_address.getDriverDecoderMask();

						Type_Decoder_STP const bio_synapse_property(
							syntype_proj, driver_dec, stp_proj);

						SynapseOnHICANN syn_addr;
						bool found_candidate = false;

						while (!found_candidate) {
							std::tie(syn_addr, found_candidate) =
							    synapses_on_vline.getSynapse(target_nrn, bio_synapse_property);

							if (!found_candidate) {
								MAROCCO_TRACE("There are no more available synapses");
								// There are no more available synapses.
								break;
							}

							auto synapse_proxy = chip.synapses[syn_addr];
							// assert that synapse has not been used otherwise
							assert(synapse_proxy.decoder == SynapseDecoderDisablingSynapse);
							// check that synapse has not been tagged as defect
							if (synapse_proxy.weight != SynapseWeight(0)) {
								MAROCCO_TRACE("Synapse was tagged as defect");
								found_candidate = false;
							}
						}

						if (!found_candidate) {
							MAROCCO_TRACE("lost a single Synapse");
							// no synapse found, so add to synapse loss
							syn_loss_proxy.addLoss(
							    src_neuron_in_proj_view, trg_neuron_in_proj_view);
							// NOTE: SJ here iterated over all remaining src neuron with the
							// same DriverDecoder
							// and marked the synapses as lost.
						} else {
							// we found a usable weight
							auto synapse_proxy = chip.synapses[syn_addr];
							synapse_proxy.decoder =
								(m_parameters.only_allow_background_events()
									? SynapseDecoder(0)
								 	: l1_address.getSynapseDecoderMask());

							// Before, here the distorted weight was stored.
							// As we postpone the weight trafo to HICANNTransformator, this
							// has to be done there (TODO: #1611)
							// Instead, we only mark the synapse as realized in the target
							// chip.
							// syn_loss_proxy.setWeight(src_neuron_in_proj_view,
							// trg_neuron_in_proj_view, clipped_weight);
							syn_loss_proxy.addRealized();

							// store synapse mapping
							m_result.synapses().add(
								results::Synapses::edge_type(proj_item.edge()),
								proj_item.projection(), source_item.bio_neuron(),
								target_item.bio_neuron(), SynapseOnWafer(syn_addr, m_hicann),
								syntype_proj, stp_proj, weight);
						}
					}
				}
			}
		} // synapse driver assignments

		MAROCCO_DEBUG(syn_manager); // print row usage stats

	} // left/right driver bank

	disableDefectSynapes();
}

void SynapseRouting::invalidateSynapseDecoder()
{
	auto& chip = m_hardware[m_hicann];

	for (auto const& row : iter_all<SynapseRowOnHICANN>())
	{
		auto proxy = chip.synapses[row];
		for (auto& dec : proxy.decoders) {
			// 0bXX0001 addresses are unused addresses
			dec = SynapseDecoderDisablingSynapse;
		}
	}
}

void SynapseRouting::tagDefectSynapses()
{
	auto& chip = m_hardware[m_hicann];
	auto const& defects = m_resource_manager.get(m_hicann);
	auto const& syns = defects->synapses();

	MAROCCO_INFO("Handling defect synapses");
	for (auto const& syn : syns->disabled())
	{
		auto proxy = chip.synapses[syn];
		proxy.weight = SynapseWeight(1);
		MAROCCO_DEBUG("Marked " << syn << " on " << m_hicann << " as defect/disabled");
	}
	// Consider using a database with blocklisted drivers in addition to their
	// connected individual blocklisted synapse rows
	// Removal of synapses through single blocklisted synapse rows is used
	// here to keep one of the two driver's rows available
	auto const& srows = defects->synapserows();
	for (auto const& srow : srows->disabled()) {
		for (auto const& scol : iter_all<SynapseColumnOnHICANN>()) {
			SynapseOnHICANN const syn(srow, scol);
			auto proxy = chip.synapses[syn];
			proxy.weight = SynapseWeight(1);
			MAROCCO_DEBUG("Marked " << syn << " on " << m_hicann << " as defect/disabled");
		}
	}
	// For defect synapse arrays the synapse drivers are disabled
}

void SynapseRouting::disableDefectSynapes()
{
	auto& chip = m_hardware[m_hicann];

	for (auto const& syn : iter_all<SynapseOnHICANN>())
	{
		auto proxy = chip.synapses[syn];
		if (proxy.decoder == SynapseDecoderDisablingSynapse) {
			proxy.weight = SynapseWeight(0);
		}
	}
}

} // namespace routing
} // namespace marocco
