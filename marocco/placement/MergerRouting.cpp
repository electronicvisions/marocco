#include "marocco/placement/MergerRouting.h"

#include <chrono>
#include <tbb/parallel_for_each.h>
#include <unordered_map>

#include "hal/Coordinate/iter_all.h"
#include "marocco/Logger.h"
#include "marocco/placement/MergerTreeRouter.h"
#include "marocco/placement/MergerTreeConfigurator.h"
#include "marocco/util.h"
#include "marocco/util/chunked.h"

using namespace HMF::Coordinate;
using marocco::assignment::PopulationSlice;

namespace marocco {
namespace placement {

namespace {

template <typename T>
void handle_defects(MergerTreeGraph& graph, HICANNOnWafer const& hicann, T const& res)
{
	for (auto it = res->begin_disabled(); it != res->end_disabled(); ++it) {
		graph.remove(*it);
		debug(&graph) << "Marked " << *it << " on " << hicann << " as defect/disabled";
	}
}

} // namespace

MergerRouting::MergerRouting(
		pymarocco::PyMarocco& pymarocco,
		graph_t const& graph,
		hardware_system_t& hw,
		resource_manager_t const& mgr) :
	mGraph(graph),
	mHW(hw),
	mMgr(mgr),
	mPyMarocco(pymarocco)
{}


void MergerRouting::run(NeuronPlacementResult const& neuronpl,
						OutputMappingResult& output_mapping)
{
	info(this) << "MergerRouting started";

	for (auto hicann : mMgr.allocated())
	{
		// create entry (if it doesn't exist yet)
		output_mapping[hicann];
	}

	auto first = mMgr.begin_allocated();
	auto last  = mMgr.end_allocated();

	auto start = std::chrono::system_clock::now();
	//tbb::parallel_for_each(first, last,
	std::for_each(first, last,
		[&](HICANNGlobal const& hicann) {
			OutputBufferMapping& local_output_mapping = output_mapping[hicann];
			NeuronBlockMapping const& nbmap = neuronpl.denmem_assignment().at(hicann);
			run(hicann, nbmap, local_output_mapping);
		});
	auto end = std::chrono::system_clock::now();
	mPyMarocco.stats.timeSpentInParallelRegion +=
		std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count();

}

void MergerRouting::run(
	HICANNGlobal const& hicann,
	NeuronBlockMapping const& nbmap,
	OutputBufferMapping& local_output_mapping)
{
	// Assign 'real' Neurons (no spike sources) to output buffers.

	MergerTreeGraph merger_graph;

	// remove defect mergers from the graph
	MAROCCO_INFO("Disabling defect mergers");
	auto const& defects = mMgr.get(hicann);
	handle_defects(merger_graph, hicann, defects->mergers0());
	handle_defects(merger_graph, hicann, defects->mergers1());
	handle_defects(merger_graph, hicann, defects->mergers2());
	handle_defects(merger_graph, hicann, defects->mergers3());
	handle_defects(merger_graph, hicann, defects->dncmergers());

	MergerTreeRouter::result_type merger_mapping;

	switch(mPyMarocco.routing.merger_tree_strategy) {
		case pymarocco::Routing::MergerTreeStrategy::minSPL1: {
			// MergerTreeRouting tries to merge adjacent neuron blocks such that the
			// overall use of SPL1 outputs is minimized.  Every unused SPL1 output can
			// then be used for external input.

			MergerTreeRouter router(merger_graph, nbmap);
			router.run();
			merger_mapping = router.result();
			break;
		} // case minSPL1
		case pymarocco::Routing::MergerTreeStrategy::maxSPL1: {
			// TODO: Instead of only reserving DNCMergerOnHICANN(7) for external input,
			// only insert neuron blocks that have neurons placed to them.
			merger_mapping[NeuronBlockOnHICANN(0)] = DNCMergerOnHICANN(0);
			merger_mapping[NeuronBlockOnHICANN(1)] = DNCMergerOnHICANN(1);
			merger_mapping[NeuronBlockOnHICANN(2)] = DNCMergerOnHICANN(2);
			merger_mapping[NeuronBlockOnHICANN(3)] = DNCMergerOnHICANN(3);
			merger_mapping[NeuronBlockOnHICANN(4)] = DNCMergerOnHICANN(4);
			merger_mapping[NeuronBlockOnHICANN(5)] = DNCMergerOnHICANN(5);
			merger_mapping[NeuronBlockOnHICANN(6)] = DNCMergerOnHICANN(6);
			break;
		} // case maxSPL1
		default:
			throw std::runtime_error("unknown merger tree strategy");
	} // switch merger tree strategy

	auto& chip = mHW[hicann];

	MergerTreeConfigurator configurator(chip.layer1, merger_graph, merger_mapping);
	configurator.run();

	for (auto const& it : merger_mapping)
	{
		NeuronBlockOnHICANN const& nb = it.first;
		DNCMergerOnHICANN const& dnc = it.second;

		// set this SPL1 merger to output
		local_output_mapping.setMode(dnc, OutputBufferMapping::OUTPUT);

		OnNeuronBlock const& onb = nbmap.at(nb);
		// Iterate over populations assigned to this neuron block
		for (auto it = onb.begin(); it != onb.end(); ++it) {
			std::shared_ptr<NeuronPlacementRequest> const& assign = *it;
			auto const& bio = assign->population_slice();
			size_t const num_neurons = bio.size();
			size_t const hw_neuron_size = assign->neuron_size();
			auto addresses = local_output_mapping.popAddresses(dnc, num_neurons, mPyMarocco.l1_address_assignment);

			local_output_mapping.insert(dnc,
				assignment::AddressMapping(bio, addresses));

			for (auto& nrn : chunked(onb.neurons(it), hw_neuron_size)) {
				HMF::HICANN::Neuron& neuron = chip.neurons[nrn.begin()->toNeuronOnHICANN(nb)];
				neuron.address(addresses.at(nrn.index()));
				neuron.activate_firing(true);
				neuron.enable_spl1_output(true);
			}
		}
	} // for all mapped NeuronBlocks
}

} // namespace placement
} // namespace marocco
