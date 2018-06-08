#include "marocco/placement/HICANNPlacement.h"
#include <unordered_map>

#include "hal/Coordinate/iter_all.h"
#include "marocco/Logger.h"
#include "marocco/util.h"
#include "marocco/partition/Metis.h"
#include "pymarocco/Placement.h"

using namespace HMF::Coordinate;

namespace {

using namespace marocco;
using namespace marocco::placement;

	//{{ 2, 4, 60, 58 }},  // neuron_size 2
	//{{ 4, 6, 60, 56 }},  // neuron_size 4
	//{{ 6, 0, 64, 64 }},  // neuron_size 6
	//{{ 8, 3, 64, 56 }}   // neuron_size 8

static std::array<std::array<size_t, 2+8>, 4> const hack = {{
	//{{ 2, 4,   60, 58, 60, 58,  60, 58, 60, 58 }},  // neuron_size 2
	{{ 2, 4,    4,  4,  4,  6,   4,  6,  4,  6 }},  // neuron_size 2
	//{{ 4, 6,   60, 56, 60, 60,  60, 56, 60, 60 }},  // neuron_size 4
	{{ 4, 6,    4,  8,  4,  4,   4,  8,  4,  4 }},  // neuron_size 4
	//{{ 6, 0,   64, 64, 64, 64,  64, 64, 64, 64 }},  // neuron_size 6
	{{ 6, 0,    0,  0,  0,  0,   0,  0,  0,  0 }},  // neuron_size 6
	//{{ 8, 3,   64, 56, 56, 56,  64, 56, 64, 56 }}   // neuron_size 8
	{{ 8, 3,    0,  8,  8,  8,   0,  8,  0,  8 }}   // neuron_size 8
}};

} // namespace

namespace marocco {
namespace placement {

HICANNPlacement::HICANNPlacement(
	pymarocco::Placement const& pl,
	graph_t const& nn,
	hardware_system_t const& hw,
	resource_manager_t& mgr,
	comm_t const& comm) :
		mPyPlacement(pl),
		mGraph(nn),
		mHW(hw),
		mMgr(mgr),
		mComm(comm)
{}

std::unique_ptr<NeuronPlacementResult>
HICANNPlacement::run()
{
	std::unique_ptr<NeuronPlacementResult> res(new NeuronPlacementResult);

	// TODO: this should in priciple be provided by redman.
	size_t const num_wafers = mHW.size();
	if (num_wafers==0) {
		throw std::runtime_error("no wafers in the system");
	} else if (num_wafers>1) {
		MAROCCO_WARN("experimental multi-wafer placement");
	}


	for (auto const& hicann : mMgr.present()) {
		auto const& defects = mMgr.get(hicann);
		auto const& nrns = defects->neurons();
		auto& neuron_placement = (*res)[hicann];

		for (auto it = nrns->begin_disabled(); it != nrns->end_disabled(); ++it) {
			auto const nb = it->toNeuronBlockOnHICANN();
			auto const nrn = it->toNeuronOnNeuronBlock();
			neuron_placement[nb].add_defect(nrn);
		}
	}


	std::unordered_map<HMF::Coordinate::Wafer, std::map<size_t, TerminalList>> hset;
	std::unordered_map<HMF::Coordinate::Wafer, set_pop> pset;
	std::vector<HMF::Coordinate::Wafer> wafers;

	// collect all available HICANNs in a set, where they are ordered in a
	// spiral from inner HICANNs to other ones.
	for (auto const& hicann : mMgr.present())
	{
		auto const& wafer = hicann.toWafer();
		for (auto const& nb : iter_all<NeuronBlockOnHICANN>())
		{
			size_t const avail = (*res)[hicann].available(nb);
			hset[wafer][avail].insert(NeuronBlockGlobal(nb, hicann));
		}

		auto it = pset.find(wafer);
		if (it==pset.end()) {
			wafers.push_back(wafer);
			pset.insert(std::make_pair(wafer, set_pop()));
		}
	}

	if (hset.empty() || pset.empty()) {
		throw std::runtime_error("no available wafer");
	}


	// now partition the network
	std::vector<int> partitioning(num_vertices(mGraph), 0);
	if (num_wafers>1) {
		// run metis only, if there is really more than 1 wafer
		Accessor<graph_t, population_t, size_t (Population::*)() const> vac(mGraph, &Population::size);
		Accessor<graph_t, projection_t, size_t (ProjectionView::*)() const> eac(mGraph, &ProjectionView::size);

		//// partiioning
		partition::Metis<graph_t> metis(mGraph, vac, eac);
		partitioning = metis.run(num_wafers);
	}


	// now collect all populations and sort dem in a descending order
	auto const& popmap = boost::get(population_t(), mGraph);
	for (auto const& v : make_iterable(boost::vertices(mGraph)))
	{
		if (is_source(v, mGraph)) {
			// we don't have to place external spike inputs
			continue;
		}

		Population const& pop = *popmap[v];

		// if a manual placement exists, use it otherwise add population to
		// `pops` list for auto placement.
		auto it = mPyPlacement.iter().find(pop.id());
		if (it != mPyPlacement.iter().end())
		{
			// get the target
			auto const& entry = mPyPlacement.iter().at(pop.id());
			size_t const hw_size = entry.second;

			// we have a target hardware size as well as a location
			std::list<HICANNGlobal> const& list = entry.first;
			if (!list.empty()) {
				MAROCCO_INFO("2 Population size: " << pop.size() << " " <<  hw_size);
				set_pop pops{
				    NeuronPlacement{assignment::PopulationSlice{v, pop}, hw_size}};
				std::list<NeuronBlockGlobal> terminals;
				for (auto const& entry : list) {
					for (auto const& nb : iter_all<NeuronBlockOnHICANN>()) {
						terminals.push_back(NeuronBlockGlobal(nb, entry));
					}
				}
				place2_(terminals, pops, *res);
			} else {
				// we only have the hardware size put bo specific place to put
				// the neurons.
				MAROCCO_INFO("1 Population size: " << pop.size() << " " <<  hw_size);
				NeuronPlacement const assign{assignment::PopulationSlice{v, pop}, hw_size};
				pset[wafers.at(partitioning.at(v))].push_back(assign);
			}
		}
		else
		{
			MAROCCO_INFO("0 Population size: " << pop.size() << " "
			                                   << mPyPlacement.getDefaultNeuronSize());
			NeuronPlacement const assign{assignment::PopulationSlice{v, pop},
			                             mPyPlacement.getDefaultNeuronSize()};
			pset[wafers.at(partitioning.at(v))].push_back(assign);
		}
	}

	for (auto const& wafer : wafers)
	{
		// do the actual placement
		place2(hset.at(wafer), pset.at(wafer), *res);
	}

	return res;
}

void HICANNPlacement::place2(
	std::map<size_t, TerminalList>& tlist,
	set_pop& pops,
	NeuronPlacementResult& res)
{
	std::list<NeuronBlockGlobal> terminals;
	for (auto const& entry : tlist)
	{
		for (auto const& e : entry.second)
		{
			terminals.push_back(e);
		}
	}

	auto const& popmap = boost::get(population_t(), mGraph);
	pops.sort([&](NeuronPlacement const& a, NeuronPlacement const& b) {
		return popmap[a.population()]->id() < popmap[b.population()]->id();
	});

	place2_(terminals, pops, res);
}


void HICANNPlacement::place2_(
	std::list<NeuronBlockGlobal>& terminals,
	set_pop& pops,
	NeuronPlacementResult& res)
{
	while (!pops.empty())
	{
		if (terminals.empty()) {
			throw std::runtime_error("not enough space on hardware (no terminals left)");
		}


		// We find the smalles suitable, so we won't fragment the
		// Wafer too much. One could also search for bigger sizes
		// than acutally needed to pack the wafer a little more
		// fluffy.
		auto it = terminals.begin();
		auto nb = it->toNeuronBlockOnHICANN();
		auto h = it->toHICANNGlobal();


		// HACK: See configureGbitLinks() in InputPlacement.cpp for explanation
		// of OutputBufferOnHICANN(7) special case. Basically that line through
		// the merger tree is reserved for DNC input + background generator events.
		bool const hack = mPyPlacement.use_output_buffer7_for_dnc_input_and_bg_hack;
		size_t const avail = (hack && nb.value() == 7) ? 0 : available(res, h, nb);


		// get first assignment and remove it from set
		NeuronPlacement assign = *(pops.begin()); // no reference!
		size_t const usable = avail / assign.neuron_size();
		MAROCCO_DEBUG("popping: " << assign.population_slice().size() << "  " << avail
		                          << " " << usable << "  " << assign.neuron_size() << " "
		                          << h << " " << nb);

		if(!usable) {
			// this happens during manual placement, if different populations
			// have been placed to same hicann
			terminals.erase(it);
			continue;
		}
		pops.erase(pops.begin());

		auto& bio = assign.population_slice();
		auto _new = NeuronPlacement{bio.slice_front(std::min(bio.size(), usable)),
		                            assign.neuron_size()};
		if (bio.size()) {
			pops.push_front(assign);
		}

		// now, try to find a sufficiently large location
		OnNeuronBlock& onb = res[h][nb];
		auto const onb_it = onb.add(_new);
		if (onb_it != onb.end()) {
			// tag HICANN as `in use` in the resource manager.
			if (mMgr.available(h)) {
				mMgr.allocate(h);
			}

			// insert mappings HW -> Bio & Bio -> HW
			// FIXME: WTF: insert does not insert both mappings!!! actually, only bio to hw placement is added!!!
			insert(h, nb, _new, *onb.neurons(onb_it).begin(), res.at(h), res.placement());

			size_t const space_left = available(res, h, nb);
			MAROCCO_DEBUG(space_left << " neurons left on " << h << " " << nb);
			if (!space_left || space_left <= assign.neuron_size()) {
				terminals.erase(it);
			}
		} else {
			// this might happen if not enough consecutive space is left due to
			// hardware defects.

			// if size is already 1, terminal is useless. Otherwise spilt
			// assignment and reinsert it.
			if (_new.population_slice().size() == 1) {
				terminals.erase(it);
				pops.push_front(_new);
			} else {
				auto const ar = _new.population_slice().split();
				for (auto const& entry: ar) {
					pops.push_front(
						NeuronPlacement{entry, _new.neuron_size()});
				}
			}
		}
	} // for all assignments
}


void HICANNPlacement::insert(
	HICANNGlobal const& hicann,
	NeuronBlockOnHICANN const& nb,
	NeuronPlacement const& assign,
	assignment::Hardware::offset_type const& offset,
	NeuronBlockMapping& hw,
	PlacementMap& revmap)
{
	auto const size = assign.size();

	if (hw.available(nb) < size)
		std::runtime_error("WTF");

	// HW -> Bio mapping
	//size_t const offset = hw.insert(nb, assign);

	// Bio -> HW mapping
	assignment::Mapping mapping;
	try {
		// try to catch copy from property map
		mapping = revmap.get(assign.population());
	} catch (std::out_of_range const&) {}

	mapping.assignment().push_back(assignment::Hardware(
	    NeuronBlockGlobal(nb, hicann), offset, size, assign.neuron_size()));
	revmap.put(assign.population(), mapping);
}

size_t HICANNPlacement::available(
	NeuronPlacementResult const& res,
	HMF::Coordinate::HICANNGlobal const& hicann,
	HMF::Coordinate::NeuronBlockOnHICANN const& nb) const
{
	static size_t const default_neuron_size = mPyPlacement.getDefaultNeuronSize();
	size_t const avail = res.at(hicann).available(nb);
	if (mPyPlacement.minSPL1 && default_neuron_size<=8)
	{
		static auto const& hack_ = hack.at(default_neuron_size/2-1);
		if (hack_[0] != default_neuron_size) {
			throw std::runtime_error("WTF");
		}
		return avail - hack_[nb.value()+2];
	}
	return avail;
}

} // namespace placement
} // namespace marocco
