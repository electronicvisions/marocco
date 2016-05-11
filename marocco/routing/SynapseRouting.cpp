#include "marocco/routing/SynapseRouting.h"

#include <cstdlib>
#include <functional>
#include <stdexcept>
#include <boost/make_shared.hpp>

#include "HMF/SynapseDecoderDisablingSynapse.h"
#include "hal/Coordinate/Synapse.h"
#include "hal/Coordinate/iter_all.h"
#include "hal/Coordinate/typed_array.h"

#include "marocco/Logger.h"
#include "marocco/routing/Fieres.h"
#include "marocco/routing/HandleSynapseLoss.h"
#include "marocco/routing/SynapseDriverRequirements.h"
#include "marocco/routing/SynapseLoss.h"
#include "marocco/routing/SynapseManager.h"
#include "marocco/routing/SynapseTargetMapping.h"
#include "marocco/routing/util.h"

// NOTE: always use clear vertex and maybe edge lists, rather than vectors,
// because we have a rather dynamically changing graph.
// Resource allocate for e.g. synapse driver will remove edges.

using namespace HMF::Coordinate;
using namespace HMF::HICANN;

namespace marocco {
namespace routing {

template <typename T>
using by_side_type = typed_array<T, SideHorizontal>;

template <typename T>
using by_vline_type = std::unordered_map<VLineOnHICANN, T>;

SynapseRouting::SynapseRouting(
	HMF::Coordinate::HICANNGlobal const& hicann,
	BioGraph const& bio_graph,
	hardware_system_t& hardware,
	resource_manager_t& resource_manager,
	parameters::SynapseRouting const& parameters,
	placement::results::Placement const& neuron_placement,
	results::L1Routing const& l1_routing,
	boost::shared_ptr<SynapseLoss> const& synapse_loss)
	: m_hicann(hicann),
	  m_bio_graph(bio_graph),
	  m_hardware(hardware),
	  m_resource_manager(resource_manager),
	  m_parameters(parameters),
	  m_neuron_placement(neuron_placement),
	  m_l1_routing(l1_routing),
	  m_synapse_loss(synapse_loss)
{
}

void SynapseRouting::run()
{
	auto& chip = m_hardware[m_hicann];

	// firstly set all SynapseDecoders to 0bXX0001 addresses
	invalidateSynapseDecoder();
	tagDefectSynapses();

	// mapping of synapse targets (excitatory, inhibitory) to synaptic inputs of denmems
	SynapseTargetMapping& syn_tgt_mapping = m_result.synapse_target_mapping;

	syn_tgt_mapping.simple_mapping(m_hicann, m_neuron_placement, m_bio_graph.graph());
	assert(syn_tgt_mapping.check_top_and_bottom_are_equal());
	MAROCCO_DEBUG("SynapseTargetMapping:\n" << syn_tgt_mapping);

	MAROCCO_INFO("calc synapse driver requirements for hicann " << m_hicann);

	SynapseDriverRequirements drivers_required(m_hicann, m_neuron_placement, syn_tgt_mapping);

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
		MAROCCO_WARN("using non-default synapse driver chain length of " << chain_length);
	}


	std::unordered_map<HMF::Coordinate::NeuronOnHICANN,
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
		std::vector<SynapseDriverOnHICANN> defect_list;
		auto const& defects = m_resource_manager.get(m_hicann);
		auto const& drvs = defects->drivers();
		MAROCCO_INFO("Collecting defect synapse drivers");
		for (auto it=drvs->begin_disabled(); it!=drvs->end_disabled(); ++it)
		{
			defect_list.push_back(*it);
			MAROCCO_DEBUG("Marked " << *it << " on " << m_hicann << " as defect/disabled");
		}

		Fieres fieres(requested_drivers, drv_side, chain_length, defect_list);
		std::vector<VLineOnHICANN> const rejected = fieres.rejected();
		auto const result = fieres.result();

		{
			size_t used = 0;
			for (auto const& item : result) {
				for (auto const& assignment : item.second) {
					used += assignment.drivers.size();
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
			std::vector<DriverAssignment> const& as = entry.second;
			auto const& route_item = route_item_by_source[drv_side].at(vline).get();
			auto const& route = route_item.route();
			DNCMergerOnWafer const route_source_merger = route_item.source();
			HICANNOnWafer const route_source_hicann = route.source_hicann();
			assert(route_item.target() == m_hicann);
			MAROCCO_TRACE(
			    "======================================================================"
			    "route from "
			    << route_source_merger << " to " << route.back() << " on "
			    << route.target_hicann());

			SynapseManager::SynapsesOnVLine synapses_on_vline =
				syn_manager.getSynapses(vline, synapse_type_to_synapse_columns_map);

			////////////////////////////////////////
			// S T O R E   D R I V E R   C O N F I G
			////////////////////////////////////////

			// generate routing result for the current route
			DriverResult driver_res(vline);

			// first insert primary and adjacent drivers
			for (DriverAssignment const& da : as)
			{
				std::vector<SynapseDriverOnHICANN>& drivers = driver_res.drivers()[da.primary];
				drivers.reserve(da.drivers.size());
				for (auto const& drv : da.drivers) {
					drivers.push_back(drv);
				}
			}

			syn_manager.check(chain_length);

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

				for(auto const& row : rows)
				{
					{
						// set stp
						SynapseDriverOnHICANN const driver = row.toSynapseDriverOnHICANN();
						auto it_stp = driver_res.stp_settings().find(driver);
						// it there already exists a config, check for equality
						// otherwise insert it
						if (it_stp != driver_res.stp_settings().end()) {
							if (it_stp->second != stp)
								throw std::runtime_error(
									"different stp settings requested on one synapse driver");
						} else {
							driver_res.stp_settings()[driver] = stp;
						}
					}

					{
						// side and decoder
						// it there already exists a SynapseRowSource config, check for equality of
						// side
						// otherwise insert it
						auto it = driver_res.rows().find(row);
						if (it != driver_res.rows().end()) {
							if (it->second.synaptic_input() != side)
								throw std::runtime_error(
									"different synaptic input side settings "
									"requested on one synapse row");
						} else {
							std::tie(it, std::ignore) = driver_res.rows().insert(
								std::make_pair(row, SynapseRowSource(side)));
						}
						it->second.prefix(static_cast<size_t>(parity)) =
							m_parameters.only_allow_background_events() ? DriverDecoder(0)
							                                            : decoder;
					}
				}
			}

			//////////////////////////////////
			// P R O C E S S   S Y N A P S E S
			//////////////////////////////////

			for (auto const& proj_item : m_l1_routing.find_projections(route_item)) {
				auto const edge = m_bio_graph.edge_from_id(proj_item.projection());
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
								// There are no more available synapses.
								break;
							}

							auto synapse_proxy = chip.synapses[syn_addr];
							// assert that synapse has not been used otherwise
							assert(synapse_proxy.decoder == SynapseDecoderDisablingSynapse);
							// check that synapse has not been tagged as defect
							if (synapse_proxy.weight != SynapseWeight(0)) {
								found_candidate = false;
							}
						}

						if (!found_candidate) {
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
							auto it = driver_res.rows().find(syn_addr.toSynapseRowOnHICANN());
							assert(it != driver_res.rows().end());
							it->second.synapses().at(syn_addr.toSynapseColumnOnHICANN()) =
							    SynapseSource(
							        proj_view, src_neuron_in_proj_view, trg_neuron_in_proj_view);
						}
					}
				}
			}

			m_result.driver_result.push_back(std::move(driver_res));
		} // synapse driver assignments

		MAROCCO_DEBUG(syn_manager); // print row usage stats

	} // left/right driver bank

	disableDefectSynapes();
}

SynapseRouting::Result const& SynapseRouting::getResult() const
{
	return m_result;
}

SynapseRouting::Result& SynapseRouting::getResult()
{
	return m_result;
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
	for (auto it=syns->begin_disabled(); it!=syns->end_disabled(); ++it)
	{
		auto proxy = chip.synapses[*it];
		proxy.weight = SynapseWeight(1);
		MAROCCO_DEBUG("Marked " << *it << " on " << m_hicann << " as defect/disabled");
	}
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
