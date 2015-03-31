#include "marocco/placement/MergerRouting.h"

#include <chrono>
#include <tbb/parallel_for_each.h>
#include <unordered_map>

#include "hal/Coordinate/iter_all.h"
#include "marocco/Logger.h"
#include "marocco/placement/MergerTree.h"
#include "marocco/util.h"

using namespace HMF::Coordinate;
using marocco::assignment::PopulationSlice;

namespace marocco {
namespace placement {

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
			NeuronBlockMapping const& nbmap = neuronpl.at(hicann);
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
	auto& chip = mHW[hicann];

	// Assign 'real' Neurons (no spike sources) to output buffers.

	MergerTreeRouter::Result merger_mapping;

	using pymarocco::PyMarocco;

	switch(mPyMarocco.routing.merger_tree_strategy) {

	case pymarocco::Routing::MergerTreeStrategy::minSPL1: {

		// MergerTreeRouting finds mergable NeuronBlock assignments such that
		// the overall use of SPL1 outputs is minimized.
		// Every unused SPL1 output can then be used for Layer2 input.
		MergerTreeRouter merger_tree(hicann, nbmap, chip, mMgr);
		//merger_tree.mergers_to_right_only();
		merger_tree.run();
		merger_mapping = merger_tree.result();

		break;

	} // case minSPL1

	case pymarocco::Routing::MergerTreeStrategy::maxSPL1: {

		merger_mapping[HMF::Coordinate::NeuronBlockOnHICANN(HMF::Coordinate::Enum(0))] = HMF::Coordinate::OutputBufferOnHICANN(HMF::Coordinate::Enum(0));
		merger_mapping[HMF::Coordinate::NeuronBlockOnHICANN(HMF::Coordinate::Enum(1))] = HMF::Coordinate::OutputBufferOnHICANN(HMF::Coordinate::Enum(1));
		merger_mapping[HMF::Coordinate::NeuronBlockOnHICANN(HMF::Coordinate::Enum(2))] = HMF::Coordinate::OutputBufferOnHICANN(HMF::Coordinate::Enum(2));
		merger_mapping[HMF::Coordinate::NeuronBlockOnHICANN(HMF::Coordinate::Enum(3))] = HMF::Coordinate::OutputBufferOnHICANN(HMF::Coordinate::Enum(3));
		merger_mapping[HMF::Coordinate::NeuronBlockOnHICANN(HMF::Coordinate::Enum(4))] = HMF::Coordinate::OutputBufferOnHICANN(HMF::Coordinate::Enum(4));
		merger_mapping[HMF::Coordinate::NeuronBlockOnHICANN(HMF::Coordinate::Enum(5))] = HMF::Coordinate::OutputBufferOnHICANN(HMF::Coordinate::Enum(5));
		merger_mapping[HMF::Coordinate::NeuronBlockOnHICANN(HMF::Coordinate::Enum(6))] = HMF::Coordinate::OutputBufferOnHICANN(HMF::Coordinate::Enum(6));
		// FIXME: only number 7 left for fpga input, change to using only buffers that really have neurons

		break;

	} // case maxSPL1

    default:

		throw std::runtime_error("unknown merger tree strategy");

	} // switch merger tree strategy

	for (auto const& it : merger_mapping)
	{
		NeuronBlockOnHICANN const& nb = it.first;
		OutputBufferOnHICANN const& outb = it.second;

		// set this SPL1 merger to output
		local_output_mapping.setMode(outb, OutputBufferMapping::OUTPUT);

		OnNeuronBlock const& onb = nbmap.at(nb);
		// Iterate over populations assigned to this neuron block
		for (auto it = onb.begin(); it != onb.end(); ++it) {
			std::shared_ptr<NeuronPlacement> const& assign = *it;
			auto const& bio = assign->population_slice();
			size_t const num_neurons = bio.size();
			size_t const hw_neuron_size = assign->neuron_size();
			auto addresses = local_output_mapping.popAddresses(outb, num_neurons, mPyMarocco.l1_address_assignment);

			local_output_mapping.insert(outb,
				assignment::AddressMapping(bio, addresses));

			size_t ii = 0;
			for (NeuronOnNeuronBlock nrn : onb.neurons(it)) {
				if (ii % hw_neuron_size == 0) {
					HMF::HICANN::Neuron& neuron = chip.neurons[nrn.toNeuronOnHICANN(nb)];
					neuron.address(addresses.at(ii / hw_neuron_size));
					neuron.activate_firing(true);
					neuron.enable_spl1_output(true);
				}
				++ii;
			}
		}
	} // for all mapped NeuronBlocks
}

} // namespace placement
} // namespace marocco
