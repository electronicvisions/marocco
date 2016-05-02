#include "marocco/placement/InputPlacement.h"

#include <cassert>
#include <set>

#include <boost/assert.hpp>
#include <nanoflann.hpp>

#include "hal/Coordinate/iter_all.h"
#include "marocco/Logger.h"
#include "marocco/placement/FiringRateVisitor.h"
#include "marocco/util/algorithm.h"
#include "marocco/util/guess_wafer.h"
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
	NeuronPlacementResult& neuron_placement,
	WaferL1AddressAssignment& address_assignment)
{
	// Assign spike inputs to remaining output buffers.

	// Good ways to do this could sort inputs by out degree and than insert them at
	// their geometric mean over all placed target populations. Or try to
	// balance input rates to minimize link saturation. For
	// SpikeSourcePoisson this is easy and for SpikeSourceArray we can
	// calculate it.

	auto const wafers = mMgr.wafers();
	BOOST_ASSERT_MSG(wafers.size() == 1, "only single-wafer use is supported");

	Neighbors<HICANNOnWafer> neighbors;
	for (auto const& hicann : mMgr.present()) {
		neighbors.push_back(hicann);
	}

	// NOTE, this is also not so nice to parallelize, because global resources
	// are assigned.
	//
	// check first if input is manually placed and assign it, then
	// collect all the inputs, get their number of target HICANNs and find the
	// optimal insertion point, given as the mean over all target HICANNs.
	auto const& plmap = neuron_placement.primary_denmems_for_population();

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
		auto it = mPyMarocco.placement.find(pop.id());
		if (it != mPyMarocco.placement.end()) {
			auto const& entry = it->second;
			std::vector<HICANNOnWafer> const* locations =
				boost::get<std::vector<HICANNOnWafer> >(&entry.locations);

			if (locations && !locations->empty()) {
				for (auto const& target_hicann : *locations) {
					insertInput(target_hicann, neuron_placement, address_assignment[target_hicann], bio);
				}

				if (bio.size()) {
					throw std::runtime_error(
						"out of resources for manually placed external inputs");
				}
			} else {
				throw std::runtime_error(
					"manual placement of external input only implemented for"
					"non-empty list of HICANNOnWafer coordinates.");
				// FIXME: ...
			}
		} else {
			// For automatic placement we compute the mean position of all target HICANNs.
			// We first have to gather all targets and remove duplicates, as they would
			// shift the mean position.

			std::set<HICANNOnWafer> targets;
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
				for (auto const& primary_neuron : locations) {
					auto const& hicann = primary_neuron.toHICANNOnWafer();
					if (targets.insert(hicann).second) {
						xs.push_back(hicann.x());
						ys.push_back(hicann.y());
					}
				}
			}

			if (targets.empty()) {
				// This may happen when a SpikeSourceArray is created and not connected to
				// any other populations, for example.
				MAROCCO_WARN("source population " << pop.id() << " does not have any targets");
				continue;
			}

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
				insertInput(target_hicann, neuron_placement, address_assignment[target_hicann], bio);

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

	for (auto const& hicann : mMgr.allocated()) {
		configureGbitLinks(hicann, address_assignment.at(hicann));
	}
}

void InputPlacement::insertInput(
	HMF::Coordinate::HICANNOnWafer const& target_hicann,
	NeuronPlacementResult& neuron_placement,
	L1AddressAssignment& address_assignment,
	PopulationSlice& bio)
{
	// 7 must be first to allow for use_output_buffer7_for_dnc_input_and_bg_hack
	for (auto const& dnc :
	     {DNCMergerOnHICANN(7), DNCMergerOnHICANN(6), DNCMergerOnHICANN(5), DNCMergerOnHICANN(4),
	      DNCMergerOnHICANN(3), DNCMergerOnHICANN(2), DNCMergerOnHICANN(1), DNCMergerOnHICANN(0)})
		{
			auto& pool = address_assignment.available_addresses(dnc);
			if (address_assignment.mode(dnc) == L1AddressAssignment::Mode::input)
				{
					size_t const left_space = pool.size();
					if (!left_space) {
						continue;
					}

					debug(this) << " found insertion point with space: " << left_space << " on: "
								<< target_hicann << " " << dnc;

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
							return;
						}
					}

					// make sure to tag HICANN as used
					{
						HICANNGlobal hicann(target_hicann, guess_wafer(mMgr));
						if (mMgr.available(hicann)) {
							mMgr.allocate(hicann);
						}
					}

					// we found an empty slot, insert the assignment
					auto population_slice = bio.slice_back(n);
					for (size_t ii = 0; ii < n; ++ii) {
						auto const address = pool.pop(mPyMarocco.l1_address_assignment);
						size_t const neuron_index = population_slice.offset() + ii;
						auto const logical_neuron = LogicalNeuron::external(
							mGraph[population_slice.population()]->id(), neuron_index);
						BioNeuron bio_neuron(population_slice.population(), neuron_index);
						neuron_placement.add(bio_neuron, logical_neuron);
						neuron_placement.set_address(
							logical_neuron,
							L1AddressOnWafer(DNCMergerOnWafer(dnc, target_hicann), address));
					}

					// mark rate as used
					if ( mPyMarocco.input_placement.consider_rate )
						allocateRate(target_hicann, used_rate);

					if (!bio.size()) {
						return;
					}
				}
		} // for all output buffer
}


void InputPlacement::configureGbitLinks(
	HICANNGlobal const& hicann,
	L1AddressAssignment& address_assignment)
{
	auto& chip = mHW[hicann];
	for (auto const dnc : iter_all<DNCMergerOnHICANN>())
	{
		GbitLinkOnHICANN const gbit_link(dnc);

		// The sending repeaters require events from the DNC mergers to arrive
		// with one idle clock cycle between two events for back-to-back
		// sending of L1 events.
		//
		// For DNC Mergers receiving events from the neuron blocks or
		// background generators, this is achieved by setting the DNC Merger to
		// slow, which however only works if the merger is set to MERGE (cf.
		// #1369).
		//
		// For DNC Mergers receiving input from Layer 2, nothing needs to be
		// done, as the pulse events arrive with a minimum interval of 56 ns
		// from the off-wafer network, which is much larger than the typical
		// duration of 2 HICANN PLL clocks (20 ns). Hence, there is no need to
		// set the merger to slow and MERGE.
		// Note that setting the merger mode to MERGE in such case can lead to
		// bad configurations of the merger tree, where events are duplicated
		// and feed back as external events into the routing (cf. #2165)

		if (address_assignment.mode(dnc) == L1AddressAssignment::Mode::output) {
			// output spikes for recording
			chip.layer1[gbit_link] = HMF::HICANN::GbitLink::Direction::TO_DNC;
			// slow only works if merger is set to MERGE
			chip.layer1[dnc] = HMF::HICANN::DNCMerger::MERGE;
			chip.layer1[dnc].slow = true;

		} else if (address_assignment.mode(dnc) == L1AddressAssignment::Mode::input) {
			// input from external FPGAs
			chip.layer1[gbit_link] = HMF::HICANN::GbitLink::Direction::TO_HICANN;
			chip.layer1[dnc] = HMF::HICANN::DNCMerger::LEFT_ONLY;
			chip.layer1[dnc].slow = false;

			// HACK: We need events with L1 address zero for locking repeaters
			// and synapse drivers. In principle those events could be provided
			// through the DNC input; But as this sets in too late and/or is too
			// short, the current approach is to use the background generator
			// connected to this output buffer (which is forwarded 1-to-1 to
			// DNC merger 7).
			bool const hack = mPyMarocco.placement.use_output_buffer7_for_dnc_input_and_bg_hack;

			if (hack && dnc == DNCMergerOnHICANN(7)) {
				Merger0OnHICANN const m(dnc.value());
				// Select (only) background generator, necessitating a hack in
				// HICANNPlacement.cpp that prevents placing neurons there.
				chip.layer1[m] = HMF::HICANN::Merger::LEFT_ONLY;
				// slow only works if merger is set to MERGE
				chip.layer1[dnc] = HMF::HICANN::DNCMerger::MERGE;
				chip.layer1[dnc].slow = true;

				MAROCCO_WARN("Neurons from right most block won't work");
			}
		} else {
			throw std::runtime_error("unknown mode");
		}
	}
}


InputPlacement::rate_type InputPlacement::availableRate(HMF::Coordinate::HICANNOnWafer const& h)
{
	// toFPGAOnWafer() is not available for HICANNOnWafer at the moment because wafer coordinate
	// is used to flag old (non-kintex) lab wafers, which have multiple reticles per FPGA.
	auto fpga = HICANNGlobal(h, guess_wafer(mMgr)).toFPGAOnWafer();
	double const& util = mPyMarocco.input_placement.bandwidth_utilization;
	rate_type avail_HICANN = util*max_rate_HICANN - mUsedRateHICANN[h];
	rate_type avail_FPGA = util * max_rate_FPGA - mUsedRateFPGA[fpga];
	rate_type avail = std::min( avail_HICANN, avail_FPGA );
	assert( avail >= 0 ); // already too much rate allocated
	return avail;
}


void InputPlacement::allocateRate(HMF::Coordinate::HICANNOnWafer const& hicann, rate_type rate)
{
	// toFPGAOnWafer() is not available for HICANNOnWafer at the moment because wafer coordinate
	// is used to flag old (non-kintex) lab wafers, which have multiple reticles per FPGA.
	auto fpga = HICANNGlobal(hicann, guess_wafer(mMgr)).toFPGAOnWafer();
	mUsedRateHICANN[hicann] += rate;
	mUsedRateFPGA[fpga] += rate;
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
