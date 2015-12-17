#include "marocco/placement/InputPlacement.h"

#include <cassert>
#include <chrono>
#include <set>

#include <boost/assert.hpp>
#include <tbb/parallel_for_each.h>
#include <nanoflann.hpp>

#include "hal/Coordinate/iter_all.h"
#include "marocco/Logger.h"
#include "marocco/placement/FiringRateVisitor.h"
#include "marocco/util/algorithm.h"
#include "marocco/util/iterable.h"
#include "marocco/util/neighbors.h"
#include "pymarocco/MappingStats.h"
#include "pymarocco/Placement.h"

using namespace HMF::Coordinate;
using marocco::assignment::PopulationSlice;

namespace marocco {
namespace placement {

namespace {

struct Point
{
	typedef float value_type;

	value_type x;
	value_type y;
};

// 7 must be first to allow for use_output_buffer7_for_dnc_input_and_bg_hack
std::vector<OutputBufferOnHICANN> const OUTBUFFERS = {
	OutputBufferOnHICANN(7),
	OutputBufferOnHICANN(6),
	OutputBufferOnHICANN(5),
	OutputBufferOnHICANN(4),
	OutputBufferOnHICANN(3),
	OutputBufferOnHICANN(2),
	OutputBufferOnHICANN(1),
	OutputBufferOnHICANN(0)
};

} // namespace

const InputPlacement::rate_type InputPlacement::max_rate_HICANN = 1.78e7; // Hz
const InputPlacement::rate_type InputPlacement::max_rate_FPGA = 1.25e8; // Hz

InputPlacement::InputPlacement(
	pymarocco::PyMarocco& pymarocco,
	graph_t const& graph,
	hardware_system_t& hw,
	resource_manager_t& mgr):
		mGraph(graph),
		mHW(hw),
		mMgr(mgr),
		mPyMarocco(pymarocco)
{
	// when considering bandwidth limitations, check utilization value
	if ( mPyMarocco.input_placement.consider_rate ) {
		double const& util = mPyMarocco.input_placement.bandwidth_utilization;
		if ( util > 0. ) {
			if (util > 1.)
				MAROCCO_WARN("bandwidth_utilization > 1. Some Input spikes will"
						" definitely be lost");
		} else {
			throw std::runtime_error("bandwidth_utilization must be positive");
		}
	}
}

void InputPlacement::run(
	NeuronPlacementResult const& neuron_mapping,
	OutputMappingResult& output_mapping)
{
	// Assign spike inputs to remaining output buffers.

	// Good ways to do this could sort inputs by out degree and than insert them at
	// their geometric mean over all placed target populations. Or try to
	// balance input rates to minimize link saturation. For
	// SpikeSourcePoisson this is easy and for SpikeSourceArray we can
	// calculate it.

	auto const wafers = mMgr.wafers();
	BOOST_ASSERT_MSG(wafers.size() == 1, "only single-wafer use is supported");

	Neighbors<HICANNGlobal> neighbors;
	for (auto const& hicann : mMgr.present()) {
		neighbors.push_back(hicann);
	}

	// NOTE, this is also not so nice to parallelize, because global resources
	// are assigned.
	//
	// check first if input is manually placed and assign it, then
	// collect all the inputs, get their number of target HICANNs and find the
	// optimal insertion point, given as the mean over all target HICANNs.
	auto const& plmap = neuron_mapping.placement();

	std::map<size_t, std::vector<std::pair<Point, PopulationSlice> >, std::greater<size_t> >
	    auto_inputs;

	for (auto const& vertex : make_iterable(boost::vertices(mGraph)))
	{
		if (!is_source(vertex, mGraph)) {
			continue;
		}

		Population const& pop = *mGraph[vertex];
		PopulationSlice bio = PopulationSlice{vertex, pop};

		// if a manual placement exists, assign it
		if (mPyMarocco.placement.iter().find(pop.id()) != mPyMarocco.placement.iter().end())
		{

			auto const& entry = mPyMarocco.placement.iter().at(pop.id());

			// FIXME: hw_size makes no sense for spike input, but we have it in the interface
			// size_t const hw_size = entry.second;

			std::list<HICANNGlobal> const& list = entry.first;

			if(!list.empty()) {
				for (auto const& target_hicann: list)
				{
					OutputBufferMapping& om = output_mapping[target_hicann];
					insertInput(target_hicann, om, bio);
				}

				if (bio.size()) {
					throw std::runtime_error("out of resources for manually placed external inputs");
				}

			} else {
				// FIXME: ...
			}
		} else {
			// For automatic placement we compute the mean position of all target HICANNs.
			// We first have to gather all targets and remove duplicates, as they would
			// shift the mean position.

			std::set<HICANNGlobal> targets;
			std::vector<float> xs, ys;
			// FIXME: Can we make a better educated guess?
			xs.reserve(out_degree(vertex, mGraph));
			ys.reserve(xs.capacity());

			for (auto const& edge : make_iterable(out_edges(vertex, mGraph))) {
				auto target = boost::target(edge, mGraph);
				if (is_source(target, mGraph)) {
					throw std::runtime_error("spike source connected to other spike source");
				}

				auto locations = plmap.at(target);
				for (auto const& loc : locations) {
					auto const& neuron_block = loc.coordinate();
					auto const& hicann = neuron_block.toHICANNGlobal();
					if (targets.insert(hicann).second) {
						xs.push_back(hicann.x());
						ys.push_back(hicann.y());
					}
				}
			}

			assert(!targets.empty());

			float const x_mean = algorithm::arithmetic_mean(xs.begin(), xs.end());
			float const y_mean = algorithm::arithmetic_mean(ys.begin(), ys.end());

			auto_inputs[targets.size()].emplace_back(Point{x_mean, y_mean}, bio);
		}
	}

	// Inputs with higher bandwidth requirements are placed first (see comparator used in
	// auto_inputs).

	for (auto& inputs_with_same_bandwidth_requirements : auto_inputs) {
		for (auto& input : inputs_with_same_bandwidth_requirements.second) {
			Point const& point = input.first;
			PopulationSlice& bio = input.second;
			if (!bio.size()) {
				throw std::runtime_error("empty input assignment");
			}

			neighbors.find_near(point.x, point.y);
			for (auto const& target_hicann : neighbors) {
				OutputBufferMapping& om = output_mapping[target_hicann];

				insertInput(target_hicann, om, bio);

				// TODO: if HICANN has no free output buffers left.
				// remove it from point cloud and rebuild index.
				// BV: Check whether this todo is still true when run
				// rate-dependent mode.

				if (!bio.size()) {
					break;
				}
			}

			if (bio.size()) {
				throw std::runtime_error("out of resources for external inputs");
			}
		}
	}

	auto first = mMgr.begin_allocated();
	auto last  = mMgr.end_allocated();

	// configure GigabitLinks on Hardware
	auto start = std::chrono::system_clock::now();
	//tbb::parallel_for_each(first, last,
	std::for_each(first, last,
		[&](HICANNGlobal const& hicann) {
			configureGbitLinks(hicann, output_mapping.at(hicann));
		});
	auto end = std::chrono::system_clock::now();
	mPyMarocco.stats.timeSpentInParallelRegion +=
		std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count();
}

void InputPlacement::insertInput(HMF::Coordinate::HICANNGlobal const& target_hicann,
								 OutputBufferMapping& om,
								 PopulationSlice& bio)
{

	for (auto const& outb : OUTBUFFERS)
		{
			if (om.getMode(outb)==OutputBufferMapping::INPUT)
				{
					size_t const left_space = om.available(outb);
					if (!left_space) {
						continue;
					}

					debug(this) << " found insertion point with space: " << left_space << " on: "
								<< target_hicann << " " << outb;

					// this is the most we get, because available resources are
					// sorted by size already.
					size_t n = std::min(bio.size(), left_space);

					rate_type used_rate;

					if ( mPyMarocco.input_placement.consider_rate ) {

						rate_type available_rate = availableRate(target_hicann);

						std::tie(n, used_rate) =
							neuronsFittingIntoAvailableRate(
								bio, n, available_rate);

						if (n == 0) {
							debug(this) << " cannot place input on this Hicann"
								<< " would exceed bandwidth limit.\n"
								<< "available: " << available_rate << " Hz,"
								<< " HICANN: " << target_hicann;
							break;
						}
					}

					// make sure to tag HICANN as used
					if (mMgr.available(target_hicann)) {
						mMgr.allocate(target_hicann);
					}

					// we found and empty slot,  insert the assignment
					auto addresses = om.popAddresses(outb, n, mPyMarocco.l1_address_assignment);
					om.insert(outb, assignment::AddressMapping(bio.slice_back(n), addresses));

					// mark rate as used
					if ( mPyMarocco.input_placement.consider_rate )
						allocateRate(target_hicann, used_rate);

					if (!bio.size()) {
						break;
					}
				}
		} // for all output buffer

}


void InputPlacement::configureGbitLinks(
	HICANNGlobal const& hicann,
	OutputBufferMapping const& output_mapping)
{
	auto& chip = mHW[hicann];
	for (auto const& outb : iter_all<OutputBufferOnHICANN>())
	{
		GbitLinkOnHICANN const c(outb);
		DNCMergerOnHICANN const dnc(outb);

		// slow only works if merger is set to MERGE
		chip.layer1[dnc] = HMF::HICANN::DNCMerger::MERGE;
		chip.layer1[dnc].slow = true;

		if (output_mapping.getMode(outb)==OutputBufferMapping::Mode::OUTPUT) {
			// output spikes for recording
			chip.layer1[c]   = HMF::HICANN::GbitLink::Direction::TO_DNC;

		} else if(output_mapping.getMode(outb)==OutputBufferMapping::Mode::INPUT) {
			// input from external FPGAs
			chip.layer1[c]   = HMF::HICANN::GbitLink::Direction::TO_HICANN;

			// If OutputBuffer is unused (represented by Mode==INPUT and empty()==true):
			// don't use MERGE and slow, if there are no sources mapped to output buffer
			// this avoids buggy configurations in the ESS (cf. #1400)
			// but has no influence on the real hardware.
			if ( output_mapping.empty(outb) ) {
				chip.layer1[dnc] = HMF::HICANN::DNCMerger::LEFT_ONLY;
				chip.layer1[dnc].slow = false;
			}

			// HACK: We need events with L1 address zero for locking repeaters
			// and synapse drivers. In principle those events could be provided
			// through the DNC input; But as this sets in too late and/or is too
			// short, the current approach is to use the background generator
			// connected to this output buffer.
			bool const hack = mPyMarocco.placement.use_output_buffer7_for_dnc_input_and_bg_hack;

			if (hack && dnc == DNCMergerOnHICANN(7)) {

				Merger0OnHICANN const m(dnc.value());
				// Select (only) background generator, necessitating a hack in
				// HICANNPlacement.cpp that prevents placing neurons there.
				chip.layer1[m] = HMF::HICANN::Merger::LEFT_ONLY;

				MAROCCO_WARN("Neurons from right most block won't work");
			} else {
				// DNCMerger always set to MERGE to allow for slow
				//chip.layer1[dnc] = HMF::HICANN::DNCMerger::LEFT_ONLY;
			}
		} else {
			throw std::runtime_error("unknown mode");
		}
	}

}


InputPlacement::rate_type InputPlacement::availableRate(
		HMF::Coordinate::HICANNGlobal const& h
		)
{
	double const& util = mPyMarocco.input_placement.bandwidth_utilization;
	rate_type avail_HICANN = util*max_rate_HICANN - mUsedRateHICANN[h];
	rate_type avail_FPGA = util*max_rate_FPGA - mUsedRateFPGA[h.toFPGAGlobal()];
	rate_type avail = std::min( avail_HICANN, avail_FPGA );
	assert( avail >= 0 ); // already too much rate allocated
	return avail;
}


void InputPlacement::allocateRate(
		HMF::Coordinate::HICANNGlobal const& hicann,
		rate_type rate)
{
	mUsedRateHICANN[hicann] += rate;
	mUsedRateFPGA[hicann.toFPGAGlobal()] += rate;
}


std::pair< size_t, InputPlacement::rate_type >
InputPlacement::neuronsFittingIntoAvailableRate(
		marocco::assignment::PopulationSlice const& bio,
		size_t max_neurons,
		rate_type available_rate
		) const
{
	Population const& pop = *mGraph[bio.population()];

	auto const& params = pop.parameters();

	FiringRateVisitor fr_visitor(mPyMarocco);

	debug(this) << " available_rate: " <<  available_rate;
	rate_type summed_rate = 0;
	size_t i = 0;
	for (; i < max_neurons; ++i) {
		// neurons are checked from the back
		size_t id_in_slice = bio.size() - i - 1;
		rate_type rate = visitCellParameterVector(
			params,
			fr_visitor,
			bio.offset() + id_in_slice
			);
		debug(this) << " exptected rate for " << id_in_slice
			<< " (ID in slice): " << rate;
		if (rate + summed_rate < available_rate)
			summed_rate += rate;
		else
			break;
	}
	return std::make_pair(i,summed_rate);
}

} // namespace placement
} // namespace marocco
