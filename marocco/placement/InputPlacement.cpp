#include <unordered_map>
#include <chrono>

#include <tbb/parallel_for_each.h>
#include <nanoflann.hpp>

#include "hal/Coordinate/iter_all.h"

#include "marocco/alg.h"
#include "marocco/placement/InputPlacement.h"
#include "marocco/util.h"
#include "marocco/Logger.h"

#include "pymarocco/MappingStats.h"
#include "pymarocco/Placement.h"

#include "marocco/placement/FiringRateVisitor.h"

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

	operator value_type const* () const
	{
		return &x;
	}

} __attribute__((packed));


struct PointCloud
{
	typedef Point::value_type value_type;

	void push_back(HICANNGlobal const& hicann)
	{
		pts.push_back(hicann);
	}

	HICANNGlobal const& at(size_t idx) const
	{
		return pts.at(idx);
	}

	size_t size() const
	{
		return pts.size();
	}

	// Must return the number of data points
	inline size_t kdtree_get_point_count() const
	{
		return pts.size();
	}

	// Returns the distance between the vector "p1[0:size-1]" and the data point with index "idx_p2" stored in the class:
	inline value_type kdtree_distance(
		value_type const* p1, size_t const idx_p2, size_t /*unused*/) const
	{
		value_type const d0=p1[0]-value_type(pts[idx_p2].x());
		value_type const d1=p1[1]-value_type(pts[idx_p2].y());
		return d0*d0+d1*d1;
	}

	inline value_type kdtree_get_pt(size_t const idx, int const dim) const
	{
		if (dim==0) {
			return value_type(pts[idx].x());
		} else {
			return value_type(pts[idx].y());
		}
	}

	template <class BBOX>
	bool kdtree_get_bbox(BBOX & /*bb*/) const
	{
		// nanoflan: Optional bounding-box computation: return false
		// to default to a standard bbox computation loop.
		return false;
	}

private:
	std::vector<HICANNGlobal> pts;
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
		mPopMap(get(population_t(), mGraph)),
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


	// prepare HICANN point clouds
	std::unordered_map<Wafer, PointCloud> point_clouds;
	for (auto const& hicann : mMgr.present())
	{
		PointCloud& pc = point_clouds[hicann.toWafer()];
		pc.push_back(hicann);
	}

	/// generate Kd-trees
	typedef nanoflann::KDTreeSingleIndexAdaptor<
		nanoflann::L2_Simple_Adaptor<Point::value_type, PointCloud> ,
		PointCloud,
		2 /* dim */
		> KDTree;
	std::unordered_map<Wafer, std::unique_ptr<KDTree>> kdtrees;
	for (auto const& entry : point_clouds)
	{
		Wafer const& wafer = entry.first;
		PointCloud const& pc = entry.second;

		kdtrees[wafer].reset(new KDTree(2 /*dim*/, pc));
		kdtrees[wafer]->buildIndex();
	}



	// NOTE, this is also not so nice to parallelize, because global resources
	// are assigned.
	//
	// check first if input is manually placed and assign it, then
	// collect all the inputs, get their number of target HICANNs and find the
	// optimal insertion point, given as the mean over all target HICANNs.
	auto const& plmap = neuron_mapping.placement();

	typedef std::map<size_t, std::vector<std::pair<Point, PopulationSlice>>,
	                 std::greater<size_t>> inputs_type;
	std::unordered_map<Wafer, inputs_type> auto_inputs;

	for (auto const& vertex : make_iterable(boost::vertices(mGraph)))
	{
		if (!is_source(vertex, mGraph)) {
			continue;
		}

		Population const& pop = *mPopMap[vertex];
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

		} else { // for auto placement

			std::unordered_map<Wafer, std::vector<float>> x;
			std::unordered_map<Wafer, std::vector<float>> y;

			// FIXME: identical target HICANNs might be considered more than once
			// and distort the mean insert location.
			size_t num_targets = 0;
			for (auto const& edge : make_iterable(out_edges(vertex, mGraph)))
			{
				auto target =  boost::target(edge, mGraph);
				if (is_source(target, mGraph)) {
					throw std::runtime_error("spike source connected to other spike source");
				}

				auto locations = plmap.get(target);
				for (auto const& loc : locations.assignment())
				{
					auto const& terminal = loc.get();
					auto const& hicann = terminal.toHICANNGlobal();

					x[hicann.toWafer()].push_back(hicann.x());
					y[hicann.toWafer()].push_back(hicann.y());

					++num_targets;
				}
			}

			// calculate mean position over all wafers
			for (auto const& entry : x)
			{
				Wafer const& wafer = entry.first;
				std::vector<float> const& _x = entry.second;
				std::vector<float> const& _y = y[wafer];

#if !defined(MAROCCO_NDEBUG)
				if (_x.empty() || _y.empty()) {
					throw std::runtime_error("found empty positions");
				}
#endif

				float const x_mean = alg::arithmetic_mean(_x.begin(), _x.end());
				float const y_mean = alg::arithmetic_mean(_y.begin(), _y.end());

				auto_inputs[wafer][num_targets].push_back(std::make_pair(
				    Point{x_mean, y_mean}, bio));
			}
		}
	}

	/// try to place inputs for all wafers in the system
	for (auto& _entry : auto_inputs)
	{
		Wafer const& wafer = _entry.first;
		inputs_type& inputs = _entry.second;

		KDTree const& kd_tree = *kdtrees[wafer];
		PointCloud const& point_cloud = point_clouds[wafer];

		// prepare a KNN ResultSet for KDTree searches
		const size_t num_results = point_clouds[wafer].size();
		std::vector<size_t> index_list(num_results);
		std::vector<Point::value_type> dist_list(num_results);


		while (!inputs.empty())
		{
			auto it = inputs.begin();
			std::vector<std::pair<Point, PopulationSlice>>& vec = it->second;

			while (!vec.empty())
			{
				// get last entry & remove it from vector
				auto e = vec.back();
				vec.pop_back();

				Point const& point = e.first;
				PopulationSlice& bio = e.second;
				if (!bio.size()) {
					throw std::runtime_error("empty input assignment");
				}

				// do a kd search
				nanoflann::KNNResultSet<Point::value_type> result(num_results);
				result.init(index_list.data(), dist_list.data());
				kd_tree.findNeighbors(result, point, nanoflann::SearchParams());

				// now find a suitable insertion point
				for (auto const idx : index_list)
				{
					HICANNGlobal const& target_hicann = point_cloud.at(idx);
					OutputBufferMapping& om = output_mapping[target_hicann];

					insertInput(target_hicann, om, bio);

					// TODO: if HICANN has no free output buffers left.
					// remove it from point cloud and rebuild index.
					// BV: Check whether this todo is still true when run
					// rate-dependent mode.

					if (!bio.size()) {
						break;
					}

				} // for neighbors

				if (bio.size()) {
					throw std::runtime_error("out of resources for external inputs");
				}
			} // inputs with same bandwidth requirements


			// DEBUG
			if (!vec.empty()) {
				// this should never happen, because loop above is only left
				// when `vec` is empty
				throw std::runtime_error("lala");
			}

			// erase `inputs` entry for this `bandwidth`. There are no more
			// inputs left with require this amount of `bandwidth`.
			inputs.erase(it);

		} // for all inputs
	} // for all wafers

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
	Population const& pop = *mPopMap[bio.population()];

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
