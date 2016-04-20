#include "marocco/routing/SynapseRouting.h"

#include <cstdlib>
#include <stdexcept>

#include "marocco/Logger.h"
#include "marocco/routing/Fieres.h"
#include "marocco/routing/SynapseDriverRequirements.h"
#include "marocco/routing/SynapseDriverSA.h"
#include "marocco/routing/SynapseLoss.h"
#include "marocco/routing/SynapseManager.h"
#include "marocco/routing/SynapseTargetMapping.h"
#include "marocco/routing/util.h"
#include "marocco/util/guess_wafer.h"

#include "hal/Coordinate/iter_all.h"

#include "HMF/SynapseDecoderDisablingSynapse.h"

#include <boost/make_shared.hpp>

// NOTE: always use clear vertex and maybe edge lists, rather than vectors,
// because we have a rather dynamically changing graph.
// Resource allocate for e.g. synapse driver will remove edges.


// One LocalRoute unwrapped
// ========================
//size_t num_sources = local_route.numSources();
//Route::BusSegment targetBus = local_route.targetBus();
//Route const& route = local_route.route();

//std::vector<HardwareProjection> const& projections = route.projections();

//for (auto const& proj : projections)
//{
	//graph_t::edge_descriptor pynn_proj = proj.projection();

	//assignment::AddressMapping const& am = proj.source();
	//std::vector<L1Address> const& addresses = am.addresses();
	//assignment::PopulationSlice const& bio = am.bio();

	//graph_t::vertex_descriptor pop = bio.population();
	//size_t size   = bio.size();
	//size_t offset = bio.offset();
//}

using namespace HMF::Coordinate;
using namespace HMF::HICANN;


namespace marocco {
namespace routing {

SynapseRouting::SynapseRouting(
	HICANNGlobal const& hicann,
	boost::shared_ptr<SynapseLoss> const& sl,
	pymarocco::PyMarocco const& pymarocco,
	graph_t const& graph,
	routing_graph const& routing_graph,
	resource_manager_t const& mgr,
	hardware_system_t& hw) :
		mPyMarocco(pymarocco),
		mHICANN(hicann),
		mHW(hw),
		mGraph(graph),
		mRoutingGraph(routing_graph),
		mManager(mgr),
		mSynapseLoss(sl)
{}

void SynapseRouting::run(placement::Result const& placement,
						 std::vector<LocalRoute> const& route_list)
{
	placement::NeuronPlacementResult const& nrnpl = placement.neuron_placement;
	auto const& revmap = nrnpl.primary_denmems_for_population();
	auto& chip = mHW[hicann()];

	// firstly set all SynapseDecoders to 0bXX0001 addresses
	invalidateSynapseDecoder();
	tagDefectSynapses();

	// mapping of synapse targets (excitatory, inhibitory) to synaptic inputs of denmems
	SynapseTargetMapping& syn_tgt_mapping = mResult.synapse_target_mapping;

	syn_tgt_mapping.simple_mapping(hicann(), nrnpl, mGraph);
	assert(syn_tgt_mapping.check_top_and_bottom_are_equal());
	std::stringstream msg;
	msg << "synapse_target_mapping:\n" << syn_tgt_mapping;
	MAROCCO_DEBUG(msg.str());

	MAROCCO_INFO("calc synapse driver requirements for hicann " << hicann());

	// secondly, generate statistics about SynapseDriver requirements
	SynapseDriverRequirements drivers_required(hicann(), nrnpl, syn_tgt_mapping);

	std::unordered_map<VLineOnHICANN, SynapseManager::Histogram> synapse_histogram;
	std::unordered_map<VLineOnHICANN, SynapseManager::Histogram> synrow_histogram;
	for (auto const& local_route : route_list)
	{
		VLineOnHICANN const vline(local_route.targetBus(mRoutingGraph).getBusId());

		std::map<Side_Parity_Decoder_STP, size_t>& synhist = synapse_histogram[vline];
		std::map<Side_Parity_Decoder_STP, size_t>& rowhist = synrow_histogram[vline];
		auto const needed = drivers_required.calc(local_route.route().projections(), mGraph, synhist, rowhist);
		MAROCCO_DEBUG(
			"route " << vline << " requires " << needed.first << " SynapseDrivers for "
					 << needed.second << " synapses");
		set(local_route, needed);
	}

	// in the following, find synapse driver assignments for driver on left and
	// right side.

	// generate two lists, of LocalRoutes seperated into vlines on left and
	// right side of hicann.
	std::array<DriverConfigurationState::IntervalList, 2> lists;

	// two mappings of VLines to route iterators, for drivers on left and right
	// side.
	std::array<std::unordered_map<VLineOnHICANN, std::vector<LocalRoute>::const_iterator>, 2> maps;

	for (auto it=route_list.begin(); it!=route_list.end(); ++it)
	{
		LocalRoute const& local_route = *it;
		L1Bus const& l1 = local_route.targetBus(mRoutingGraph);
		HICANNGlobal const& h = l1.hicann();
		VLineOnHICANN const& vline = l1.toVLine();

		auto const needed = get(local_route);

		// this can only happen in very rare ocasions, where by chance, no
		// weight has been realized in the projectionView. More likely for lower
		// connection probs.
		if (!needed.first) {
			warn(this) << "no synapse driver needed";
			handleSynapseLoss(local_route, nrnpl);
			continue;
		}

		bool side;
		if (h != hicann()) {
			// input from adjacent HICANN
			side = vline<VLineOnHICANN::end/2;
		} else {
			// input from same HICANN
			side = vline.toSideHorizontal();
		}

		lists[side].push_back(DriverInterval(vline, needed.first, needed.second));
		auto res = maps[side].insert(std::make_pair(vline, it));
		if (!res.second) {
			throw std::runtime_error("insertion error");
		}
	}


	size_t const chain_length = mPyMarocco.routing.syndriver_chain_length;
	if (chain_length != 56) {
		warn(this) << "using non-default synapse driver chain length of "
			<< chain_length;
	}


	std::unordered_map<HMF::Coordinate::NeuronOnHICANN,
					   std::map<SynapseType, SynapseDriverRequirements::SynapseColumnsMap> >
		synapse_type_to_synapse_columns_map =
			drivers_required.get_synapse_type_to_synapse_columns_map();

	// then optimize synapse driver assignment
	for (auto const& side : iter_all<SideHorizontal>())
	{
		// if list is empty, there is nothing to optimize
		DriverConfigurationState::IntervalList const& list = lists[side];
		if (list.empty()) {
			continue;
		}

		// do the actual optimization. Use simulated annealing to minimize the
		// synapse driver overlap in favor of larger assignments.
		//DriverConfigurationState state(list, side, chain_length, 42[>seed<]);
		//Annealer<DriverConfigurationState> annealer(state);
		//annealer.run(60000);

		//MAROCCO_DEBUG(annealer.iterations() << " annealing iterations");

		//DriverConfigurationState result_state = *annealer.get();
		//std::vector<VLineOnHICANN> const rejected = result_state.postProcess();
		//auto const result = result_state.result(hicann());

		// prepare defect list first.
		std::vector<SynapseDriverOnHICANN> defect_list;
		auto const& defects = mManager.get(hicann());
		auto const& drvs = defects->drivers();
		MAROCCO_INFO("Collecting defect synapse drivers");
		for (auto it=drvs->begin_disabled(); it!=drvs->end_disabled(); ++it)
		{
			defect_list.push_back(*it);
			MAROCCO_DEBUG("Marked " << *it << " on " << hicann() << " as defect/disabled");
		}

		Fieres fieres(list, side, chain_length, defect_list);
		std::vector<VLineOnHICANN> const rejected = fieres.rejected();
		auto const result = fieres.result(hicann());


		// mark synapses lost beloning to rejected incoming routes
		for (auto const& vline : rejected)
		{
			LocalRoute const& local_route = *maps[side].at(vline);
			handleSynapseLoss(local_route, nrnpl);
		} // for all rejected


		// manages the SynapseRows assigned to local routes but it
		// differentiates between sources with different MSBs and different
		// synapse types (exc/inh).
		SynapseManager syn_manager(result);
		syn_manager.init(synapse_histogram, synrow_histogram);

		for (auto const& entry : result)
		{
			VLineOnHICANN const& vline = entry.first;
			std::vector<DriverAssignment> const& as = entry.second;
			LocalRoute const& local_route = *maps[side].at(vline);

			SynapseManager::SynapsesOnVLine synapses_on_vline =
				syn_manager.getSynapses(vline, synapse_type_to_synapse_columns_map);

			////////////////////////////////////////
			// S T O R E   D R I V E R   C O N F I G
			////////////////////////////////////////

			// generate routing result for the current local_route
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
							mPyMarocco.only_bkg_visible ? DriverDecoder(0) : decoder;
					}
				}
			}

			//////////////////////////////////
			// P R O C E S S   S Y N A P S E S
			//////////////////////////////////


			//size_t num_sources = local_route.numSources();
			//Route::BusSegment targetBus = local_route.targetBus(); // unecessary here
			Route const& route = local_route.route();
			L1Bus const& l1 = mRoutingGraph[route.source()];
			HICANNGlobal const& src_hicann = l1.hicann();

			std::vector<HardwareProjection> const& projections = route.projections();

			for (auto const& proj : projections)
			{
				graph_t::edge_descriptor pynn_proj = proj.projection();

				graph_t::vertex_descriptor target = boost::target(pynn_proj, mGraph);

				assignment::AddressMapping const& am = proj.source();
				std::vector<L1Address> const& addresses = am.addresses();

				assignment::PopulationSlice const& src_bio_assign = am.bio();

				size_t const src_bio_size   = src_bio_assign.size();
				size_t const src_bio_offset = src_bio_assign.offset();


				// now there is everything from the source, now need more info about target
				// population placement.


				boost::shared_ptr<ProjectionView> const proj_view =
					boost::make_shared<ProjectionView>(mGraph[pynn_proj]);
				Population const& target_pop = *mGraph[target];


				// calculate offsets for pre population in this view
				size_t const src_neuron_offset_in_proj_view =
					getPopulationViewOffset(src_bio_offset, proj_view->pre().mask());

				// get synloss proxy object for faster loss counting
				SynapseLossProxy syn_loss_proxy =
					mSynapseLoss->getProxy(pynn_proj, src_hicann, hicann());

				for (auto const& primary_neuron : revmap.at(target)) {
					auto const terminal = primary_neuron.toNeuronBlockOnWafer();
					if (terminal.toHICANNOnWafer() != hicann().toHICANNOnWafer()) {
						// this terminal doesn't correspond to the current local
						// hicann. so there is nothing to do.
						continue;
					}

					placement::OnNeuronBlock const& onb = nrnpl.denmem_assignment().at(
						terminal.toHICANNOnWafer())[terminal.toNeuronBlockOnHICANN()];
					auto const it = onb.get(primary_neuron.toNeuronOnNeuronBlock());
					assert(it != onb.end());

					{
						// FIXME: Confirm and remove this:
						NeuronOnNeuronBlock first = *onb.neurons(it).begin();
						assert(first == primary_neuron.toNeuronOnNeuronBlock());
					}

					NeuronOnNeuronBlock const& first = primary_neuron.toNeuronOnNeuronBlock();
					std::shared_ptr<placement::NeuronPlacementRequest> const& trg_assign = *it;
					assignment::PopulationSlice const& trg_bio_assign = trg_assign->population_slice();

					size_t const trg_bio_size   = trg_bio_assign.size();
					size_t const trg_bio_offset = trg_bio_assign.offset();
					// FIXME: first.toNeuronOnHICANN(nb)...!
					size_t const trg_pop_hw_offset =
					    terminal.toNeuronBlockOnHICANN().value() * 32 + first.x();

					size_t const hw_neuron_width = trg_assign->neuron_width();

					auto bio_weights = proj_view->getWeights(); // this is just a view, no copying

					SynapseType syntype_proj = toSynapseType(proj_view->projection()->target());

					auto dynamics = proj_view->projection()->dynamics();
					STPMode stp_proj = toSTPMode(dynamics);

					// calculate offsets for post population in this view
					size_t const trg_neuron_offset_in_proj_view =
						getPopulationViewOffset(trg_bio_offset, proj_view->post().mask());

					// TODO: switch back to iterator later on. But for now
					// counting the neuron indexes and indexing the matrix
					// every time is safer and easier to debug.
					//boost::numeric::ublas::matrix_row<Connector::const_matrix_view_type> row(bio_weights, src_neuron_in_proj_view);
					//auto weight_iterator = row.begin()+trg_neuron_offset_in_proj_view ;
					size_t trg_neuron_in_proj_view = trg_neuron_offset_in_proj_view;
					for (size_t trg_neuron=trg_bio_offset; trg_neuron<trg_bio_offset+trg_bio_size; ++trg_neuron)
					{
						if (!proj_view->post().mask()[trg_neuron]) {
							continue;
						}


						size_t const trg_hw_offset = trg_pop_hw_offset+(trg_neuron-trg_bio_offset)*hw_neuron_width;

						size_t address_cnt = 0;
						size_t src_neuron_in_proj_view = src_neuron_offset_in_proj_view;
						for (size_t src_neuron=src_bio_offset; src_neuron<src_bio_offset+src_bio_size; ++src_neuron)
						{
							size_t address = address_cnt++;
							// source neuron address (relative to Population) is
							// ONLY needed for mask. To address weights, always use
							// neuron address relative to PopulationView.
							if (!proj_view->pre().mask()[src_neuron]) {
								continue;
							}

							L1Address const& l1_address = addresses.at(address);
							DriverDecoder const driver_dec = l1_address.getDriverDecoderMask();


							//double const weight = *(weight_iterator++);
#if !defined(MAROCCO_NDEBUG)
							//if (src_neuron_in_proj_view>=bio_weights.size1()) {
								//throw std::runtime_error("src out of range");
							//}
							//if (trg_neuron_in_proj_view>=bio_weights.size2()) {
								//throw std::runtime_error("trg out of range");
							//}
#endif // MAROCCO_NDEBUG
							double weight = bio_weights(src_neuron_in_proj_view, trg_neuron_in_proj_view);
							if (weight>0.)
							{
								Type_Decoder_STP const bio_synapse_property(
									syntype_proj, driver_dec, stp_proj);
								// address of leftmost top denmem of target neuron
								NeuronOnHICANN const target_nrn(X(trg_hw_offset), Y(0));

								SynapseOnHICANN syn_addr;
								bool success = false;
								while (true) {
									bool found_candidate;
									std::tie(syn_addr, found_candidate) =
										synapses_on_vline.getSynapse(
											target_nrn, bio_synapse_property);
									MAROCCO_DEBUG(
										"found syn = " << found_candidate << ", " << syn_addr);

									if (found_candidate) {
										auto synapse_proxy = chip.synapses[syn_addr];
										// assert that synapse has not been used otherwise
										assert(
											synapse_proxy.decoder ==
											SynapseDecoderDisablingSynapse);
										// check that synapse has not been tagged as defect
										if (synapse_proxy.weight == SynapseWeight(0)) {
											//  we found usable weight
											success = true;
											break;
										}
										// else try next available synapse

									} else {
										success = false;
										break;
									}
								}

								if (!success) {
									// no synapse found, so add to synapse loss
									syn_loss_proxy.addLoss(
										src_neuron_in_proj_view, trg_neuron_in_proj_view);
									// NOTE: SJ here iterated over all remaining src neuron with the
									// same DriverDecoder
									// and marked the synapses as lost.
								} else {
									// we found a usable weight
									auto synapse_proxy = chip.synapses[syn_addr];
									MAROCCO_TRACE(
										"setting synapse (" << syn_addr << ") = "
															<< l1_address.getSynapseDecoderMask());
									synapse_proxy.decoder =
										mPyMarocco.only_bkg_visible
											? SynapseDecoder(0)
											: l1_address.getSynapseDecoderMask();

									// Before, here the distorted weight was stored.
									// As we postpone the weight trafo to HICANNTransformator, this
									// has to be done there (TODO: #1611)
									// Instead, we only mark the synapse as realized in the target
									// chip.
									// syn_loss_proxy.setWeight(src_neuron_in_proj_view,
									// trg_neuron_in_proj_view, clipped_weight);
									syn_loss_proxy.addRealized();

									// store synapse mapping
									auto it =
										driver_res.rows().find(syn_addr.toSynapseRowOnHICANN());
									assert(it != driver_res.rows().end());
									it->second.synapses().at(syn_addr.toSynapseColumnOnHICANN()) =
										SynapseSource(
											proj_view, src_neuron_in_proj_view,
											trg_neuron_in_proj_view);
								}
							} // if valid bio weights
							++src_neuron_in_proj_view;
						} // src bio neurons
						++trg_neuron_in_proj_view;
					} // trg bio neurons

				} // all hw assignments
			} // all hw projections


			mResult.driver_result.push_back(std::move(driver_res));

		} // synapse driver assignments

		MAROCCO_DEBUG(syn_manager); // print row usage stats

	} // left/right driver bank

	disableDefectSynapes();
}

void SynapseRouting::set(LocalRoute const& route, std::pair<size_t, size_t> const& need)
{
	mNumSynapseDriver[route.id()] = need;
}

std::pair<size_t, size_t> SynapseRouting::get(LocalRoute const& route) const
{
	return mNumSynapseDriver.at(route.id());
}

SynapseRouting::Result const& SynapseRouting::getResult() const
{
	return mResult;
}

SynapseRouting::Result& SynapseRouting::getResult()
{
	return mResult;
}

void SynapseRouting::handleSynapseLoss(LocalRoute const& local_route,
									   placement::NeuronPlacementResult const& neuron_placement)
{
	Route const& route = local_route.route();
	L1Bus const& l1 = mRoutingGraph[route.source()];
	HICANNGlobal const& src_hicann = l1.hicann();
	std::vector<HardwareProjection> const& projections = route.projections();
	for (auto const& proj : projections)
	{
		graph_t::edge_descriptor pynn_proj = proj.projection();

		graph_t::vertex_descriptor target = boost::target(pynn_proj, mGraph);

		assignment::AddressMapping const& am = proj.source();
		assignment::PopulationSlice const& src_bio_assign = am.bio();

		for (auto const& item : neuron_placement.find(target)) {
			auto neuron_block = item.neuron_block();
			assert(neuron_block != boost::none);
			if (neuron_block->toHICANNOnWafer() != hicann().toHICANNOnWafer()) {
				// This neuron of the target population was not placed to the current hicann,
				// so there is nothing to do.
				continue;
			}

			HICANNGlobal trg_hicann(neuron_block->toHICANNOnWafer(), guess_wafer(mManager));
			mSynapseLoss->addLoss(
			    pynn_proj, src_hicann, trg_hicann, src_bio_assign,
			    assignment::PopulationSlice(item.population(), item.neuron_index(), 1));
		}
	} // for all projetions
}

void SynapseRouting::invalidateSynapseDecoder()
{
	auto& chip = mHW[hicann()];

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
	auto& chip = mHW[hicann()];
	auto const& defects = mManager.get(hicann());
	auto const& syns = defects->synapses();

	MAROCCO_INFO("Handling defect synapses");
	for (auto it=syns->begin_disabled(); it!=syns->end_disabled(); ++it)
	{
		auto proxy = chip.synapses[*it];
		proxy.weight = SynapseWeight(1);
		MAROCCO_DEBUG("Marked " << *it << " on " << hicann() << " as defect/disabled");
	}
}

void SynapseRouting::disableDefectSynapes()
{
	auto& chip = mHW[hicann()];

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
