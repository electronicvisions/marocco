#include "marocco/placement/NeuronPlacement.h"

#include <boost/assert.hpp>
#include <boost/variant.hpp>
#include <unordered_map>

#include "hal/Coordinate/iter_all.h"
#include "hal/Coordinate/typed_array.h"
#include "marocco/Logger.h"
#include "marocco/util.h"
#include "marocco/util/guess_wafer.h"
#include "marocco/util/iterable.h"
#include "pymarocco/Placement.h"

using namespace HMF::Coordinate;

namespace {

/**
 * @brief Used to extract coordinates of neuron blocks used in manual placement instructions.
 * As populations can be placed to both specific neuron blocks or HICANNs we use a visitor
 * to convert both instructions to the more specific case of a list of neuron blocks.
 */
class NeuronBlockLocationVisitor : public boost::static_visitor<>
{
public:
	/**
	 * @param[out] neuron_blocks Neuron blocks corresponding to the coordinates passed
	 *                           via \c operator().
	 */
	NeuronBlockLocationVisitor(std::vector<NeuronBlockOnWafer>& neuron_blocks)
		: m_neuron_blocks(neuron_blocks)
	{
	}
	std::vector<NeuronBlockOnWafer>& m_neuron_blocks;

	void operator()(std::vector<HICANNOnWafer> const& hicanns)
	{
		m_neuron_blocks.reserve(hicanns.size() * NeuronBlockOnHICANN::size);
		for (auto const& h : hicanns) {
			for (auto const& nb : iter_all<NeuronBlockOnHICANN>()) {
				m_neuron_blocks.push_back(NeuronBlockOnWafer(nb, h));
			}
		}
	}

	void operator()(std::vector<NeuronBlockOnWafer> const& blocks)
	{
		m_neuron_blocks = blocks;
	}
};

} // anonymous

namespace marocco {
namespace placement {

NeuronPlacement::NeuronPlacement(
    pymarocco::Placement const& pl,
    graph_t const& nn,
    hardware_system_t const& hw,
    resource_manager_t& mgr)
    : mPyPlacement(pl), mGraph(nn), mHW(hw), mMgr(mgr)
{
}

void NeuronPlacement::disable_defect_neurons(NeuronPlacementResult& res)
{
	MAROCCO_INFO("Disabling defect neurons");
	if (mPyPlacement.use_output_buffer7_for_dnc_input_and_bg_hack) {
		MAROCCO_INFO("Using OutputBuffer(7) hack");
	}
	if (mPyPlacement.minSPL1) {
		MAROCCO_INFO("Using minSPL1 option");
	}

	for (auto const& hicann : mMgr.present()) {
		auto const& neurons = mMgr.get(hicann)->neurons();

		for (auto it = neurons->begin_disabled(); it != neurons->end_disabled(); ++it) {
			auto const nb = it->toNeuronBlockOnHICANN();
			auto const nrn = it->toNeuronOnNeuronBlock();
			res[hicann][nb].add_defect(nrn);
			MAROCCO_DEBUG("Marked " << *it << " on " << hicann << " as defect/disabled");
		}

		// Reserve rightmost OutputBuffer to be used for DNC input and bg events.
		// (see comments in InputPlacement.cpp)
		if (mPyPlacement.use_output_buffer7_for_dnc_input_and_bg_hack) {
			res[hicann][NeuronBlockOnHICANN(7)].restrict(0);
		}

		/* Minimize number of required SPL repeaters.
		 * ,---- [ Thesis S. Jeltsch ]
		 * | By default, the number of available hardware neuron
		 * | circuits per chip is artificially constrained for hardware
		 * | neuron sizes of 4 and 8 to increase the Layer 1 address
		 * | utilization, as explained in Section 5.3.3.  Fewer SPL1
		 * | repeaters are required in the subsequent merger routing
		 * | stage to route events from all neurons off chip, if the
		 * | number of placed model neurons is reduced to 118 and 59 for
		 * | 4 and 8-circuit configuration, respectively.  Experiments
		 * | have shown that this is generally beneficial to improve the
		 * | synaptic loss.  However, can be turned off to increase the
		 * | number of available neuron circuits.
		 * `----
		 *
		 * ,---- [ Thesis S. Jeltsch ]
		 * | Theoretically, 6 bit Layer 1 events from 64 sources can be
		 * | merged in order to save routing resources.  However, some
		 * | addresses have a special purpose, i.e., address 0 is used
		 * | for repeater locking (see Section 1.5.5) and four more
		 * | addresses are reserved to implement disabled synapses.
		 * `----
		 *
		 * => Thus we place 5 less neurons than possible for the
		 *    default neuron size (8).
		 */

		size_t const default_neuron_size = mPyPlacement.getDefaultNeuronSize();
		assert(default_neuron_size % 2 == 0);

		// BV: “Note that in all cases, we could decrease the denmem counts by "neuron
		// size - 1" for each neuron block. To allow for a higher flexibility when
		// individual denmems are defect.”

		static std::array<typed_array<size_t, NeuronBlockOnHICANN>, 4> const restrictions = {{
			// Neuron size 2: disable 40 denmems ≙ 20 nrns ⇒ 236 nrns remaining
			// Will fit on four L1 buses.
			{{4, 6, 4, 6, 4, 6, 4, 6}},
			// Neuron size 4: disable 40 denmems ≙ 10 nrns ⇒ 118 nrns remaining
			// Will fit on two L1 buses when merged to DNCMergerOnHICANN 3 and 5.
			{{4, 8, 4, 4, 4, 8, 4, 4}},
			// Neuron size 6: max. 10 nrns per block       ⇒ 80 nrns in total
			// Will fit on two L1 buses when merged to DNCMergerOnHICANN 3 and 5.
			{{0, 0, 0, 0, 0, 0, 0, 0}},
			// Neuron size 8: disable 40 denmems ≙  5 nrns ⇒ 59 nrns remaining
			// Will fit on one L1 bus when merged to DNCMergerOnHICANN 3.
			{{0, 8, 8, 8, 0, 8, 0, 8}}
		}};

		if (mPyPlacement.minSPL1 && default_neuron_size <= 8) {
			auto const& restriction = restrictions.at(default_neuron_size / 2 - 1);
			for (auto nb : iter_all<NeuronBlockOnHICANN>()) {
				res[hicann][nb].restrict(NeuronOnNeuronBlock::enum_type::size - restriction[nb]);
			}
		}
	}
}

std::vector<NeuronPlacementRequest> NeuronPlacement::manual_placement(NeuronPlacementResult& res)
{
	std::vector<NeuronPlacementRequest> auto_placements;

	MAROCCO_INFO("Checking for manually placed populations");
	for (auto const& v : make_iterable(boost::vertices(mGraph))) {
		if (is_source(v, mGraph)) {
			// We don't have to place external spike inputs.
			continue;
		}

		Population const& pop = *mGraph[v];
		size_t const default_hw_neuron_size = mPyPlacement.getDefaultNeuronSize();

		// Check for existence of manual placement:
		auto it = mPyPlacement.find(pop.id());
		if (it != mPyPlacement.end()) {
			auto const& entry = it->second;
			NeuronPlacementRequest const placement{
			    assignment::PopulationSlice{v, pop},
			    entry.hw_neuron_size > 0 ? entry.hw_neuron_size : default_hw_neuron_size};
			std::vector<NeuronBlockOnWafer> neuron_blocks;
			{
				NeuronBlockLocationVisitor vis(neuron_blocks);
				boost::apply_visitor(vis, entry.locations);
			}
			std::vector<NeuronPlacementRequest> queue{placement};
			PlacePopulations placer(res, neuron_blocks, queue);
			auto const& result = placer.run();
			post_process(res, result);

			// The population (or parts of it) may not have been placed successfully.
#if 1
			if (!queue.empty()) {
				throw ResourceExhaustedError("unable to implement manual placement request");
			}
#else
			// Try again during auto-placement.
			std::copy(queue.begin(), queue.end(), std::back_inserter(auto_placements));
#endif
		} else {
			NeuronPlacementRequest const placement{assignment::PopulationSlice{v, pop},
			                                default_hw_neuron_size};
			auto_placements.push_back(placement);
		}
	}

	return auto_placements;
}

void NeuronPlacement::run(NeuronPlacementResult& res)
{
	auto const wafers = mMgr.wafers();
	BOOST_ASSERT_MSG(wafers.size() == 1, "only single-wafer use is supported");

	disable_defect_neurons(res);

	auto auto_placements = manual_placement(res);
	std::sort(
		auto_placements.begin(), auto_placements.end(),
		[&](NeuronPlacementRequest const& a, NeuronPlacementRequest const& b) -> bool {
			return mGraph[a.population()]->id() < mGraph[b.population()]->id();
		});

	std::vector<NeuronBlockOnWafer> neuron_blocks;
	neuron_blocks.reserve(mMgr.count_available() * NeuronBlockOnHICANN::size);

	for (auto const& hicann : mMgr.present()) {
		for (auto const& nb : iter_all<NeuronBlockOnHICANN>()) {
			neuron_blocks.emplace_back(nb, hicann);
		}
	}

	PlacePopulations placer(res, neuron_blocks, auto_placements);
	auto const& result = placer.sort_and_run();
	post_process(res, result);

	if (!auto_placements.empty()) {
		throw ResourceExhaustedError("unable to place all populations");
	}

	MAROCCO_INFO("Placement of populations finished");
}

void NeuronPlacement::post_process(
	NeuronPlacementResult& res, std::vector<PlacePopulations::result_type> const& placements)
{
	for (auto const& entry : placements) {
		auto nrn = entry.neuron;
		auto placement = entry.chunk;

		// Fill reverse lookup.
		res.placement()[placement.population()].push_back(nrn);

		// Tag HICANN as 'in use' in the resource manager.
		HICANNGlobal hicann(nrn.toHICANNOnWafer(), guess_wafer(mMgr));
		if (mMgr.available(hicann)) {
			mMgr.allocate(hicann);
		}
	}
}

} // namespace placement
} // namespace marocco
