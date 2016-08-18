#include "marocco/placement/NeuronPlacement.h"

#include <algorithm>
#include <boost/assert.hpp>
#include <boost/variant.hpp>
#include <unordered_map>

#include "hal/Coordinate/iter_all.h"
#include "hal/Coordinate/typed_array.h"

#include "marocco/Logger.h"
#include "marocco/util.h"
#include "marocco/util/chunked.h"
#include "marocco/util/iterable.h"

using namespace HMF::Coordinate;
using marocco::placement::internal::NeuronPlacementRequest;
using marocco::placement::internal::PlacePopulations;

namespace marocco {
namespace placement {

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

	void operator()(std::vector<LogicalNeuron> const&)
	{
		assert(false);
	}
};

} // anonymous

NeuronPlacement::NeuronPlacement(
	graph_t const& graph,
	parameters::NeuronPlacement const& parameters,
	parameters::ManualPlacement const& manual_placement,
	results::Placement& result,
	internal::Result& internal)
	: m_graph(graph),
	  m_parameters(parameters),
	  m_manual_placement(manual_placement),
	  m_result(result),
	  m_internal(internal),
	  m_denmem_assignment(internal.denmem_assignment)
{
}

void NeuronPlacement::add(HMF::Coordinate::HICANNOnWafer const& hicann)
{
	m_denmem_assignment[hicann];
}

void NeuronPlacement::add_defect(
    HMF::Coordinate::HICANNOnWafer const& hicann, HMF::Coordinate::NeuronOnHICANN const& neuron)
{
	MAROCCO_DEBUG("Marked " << neuron << " on " << hicann << " as defect/disabled");
	m_denmem_assignment.at(hicann)[neuron.toNeuronBlockOnHICANN()].add_defect(
	    neuron.toNeuronOnNeuronBlock());
}

void NeuronPlacement::restrict_rightmost_neuron_blocks() {
	MAROCCO_INFO("Reserving rightmost neuron blocks for bg events and DNC input");
	for (auto& item : m_denmem_assignment) {
		item.second[NeuronBlockOnHICANN(NeuronBlockOnHICANN::max)].restrict(0);
	}
}

void NeuronPlacement::minimize_number_of_sending_repeaters() {
	size_t const default_neuron_size = m_parameters.default_neuron_size();
	assert(default_neuron_size % 2 == 0);
	if (default_neuron_size > 8) {
		// For even neuron sizes ≥ 10 there are at most 6 neurons per neuron block / 48
		// neurons in total.  All blocks could thus be merged and fit on a single L1 bus.
		return;
	}

	MAROCCO_INFO("Trying to minimize number of required sending repeaters");
	for (auto& item : m_denmem_assignment) {
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

		// BV: “Note that in all cases, we could decrease the denmem counts by "neuron
		// size - 1" for each neuron block. To allow for a higher flexibility when
		// individual denmems are defect.”

		// clang-format off
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
		// clang-format on

		auto const& restriction = restrictions.at(default_neuron_size / 2 - 1);
		for (auto nb : iter_all<NeuronBlockOnHICANN>()) {
			item.second[nb].restrict(NeuronOnNeuronBlock::enum_type::size - restriction[nb]);
		}
	}
}

std::vector<NeuronPlacementRequest> NeuronPlacement::perform_manual_placement()
{
	std::vector<NeuronPlacementRequest> auto_placements;
	size_t const default_hw_neuron_size = m_parameters.default_neuron_size();
	auto const& mapping = m_manual_placement.mapping();

	MAROCCO_INFO("Checking for manually placed populations");

	for (auto const& v : make_iterable(boost::vertices(m_graph))) {
		if (is_source(v, m_graph)) {
			// We don't have to place external spike inputs.
			continue;
		}

		Population const& pop = *m_graph[v];

		// Check for existence of manual placement:
		auto it = mapping.find(pop.id());
		if (it != mapping.end()) {
			auto const& entry = it->second;

			{ // A population may be explicitly placed to a set of logical neurons.
				std::vector<LogicalNeuron> const* logical_neurons =
				    boost::get<std::vector<LogicalNeuron> >(&entry.locations);

				if (logical_neurons) {
					MAROCCO_DEBUG(
						"population " << v << " will be placed on manually specified neurons");

					if (logical_neurons->size() != pop.size()) {
						throw std::runtime_error(
							"number of logical neurons does not match size of population");
					}

					size_t neuron_id = 0;
					std::vector<NeuronOnWafer> placements;
					for (auto const& logical_neuron : *logical_neurons) {
						if (logical_neuron.is_external()) {
							throw std::runtime_error(
								"external neuron used in manual placement request");
						}

						MAROCCO_TRACE(logical_neuron);

						auto const neuron = logical_neuron.front();
						size_t const size = logical_neuron.size();

						if (size < 2 || size % 2 != 0 || !logical_neuron.is_rectangular()) {
							throw std::runtime_error(
								"manual placement only supports rectangular neurons");
						}

						NeuronPlacementRequest const placement{
							assignment::PopulationSlice{v, neuron_id, 1}, size};
						auto& onb = m_denmem_assignment.at(
							neuron.toHICANNOnWafer())[neuron.toNeuronBlockOnHICANN()];
						auto it = onb.add(neuron.toNeuronOnNeuronBlock().x(), placement);
						if (it == onb.end()) {
							throw std::runtime_error(
								"specified denmems are occupied or marked as defect");
						}
						placements.push_back(neuron);

						++neuron_id;
					}

					post_process(placements);
					continue;
				}
			}

			// Else manual placement requests specify candidate neuron blocks that can be
			// used to place a population.

			NeuronPlacementRequest const placement{
			    assignment::PopulationSlice{v, pop},
			    entry.hw_neuron_size > 0 ? entry.hw_neuron_size : default_hw_neuron_size};
			std::vector<NeuronBlockOnWafer> neuron_blocks;
			{
				NeuronBlockLocationVisitor vis(neuron_blocks);
				boost::apply_visitor(vis, entry.locations);
			}

			if (neuron_blocks.empty()) {
				// `with_size` placement request.
				auto_placements.push_back(placement);
				continue;
			}

			MAROCCO_DEBUG(placement.population_slice() << " will be placed manually");

			if (MAROCCO_TRACE_ENABLED()) {
				for (auto const& nb : neuron_blocks) {
					MAROCCO_TRACE("  on " << nb);
				}
			}

			// As PlacePopulations starts at the back of neuron_blocks for
			// placing neurons, the order of neuron blocks is reversed, so that
			// the order specified by the user is complied with.
			std::reverse(neuron_blocks.begin(), neuron_blocks.end());

			std::vector<NeuronPlacementRequest> queue{placement};
			PlacePopulations placer(m_denmem_assignment, neuron_blocks, queue);
			auto const& result = placer.run();
			post_process(result);

			// The population (or parts of it) may not have been placed successfully.
#if 1
			if (!queue.empty()) {
				MAROCCO_ERROR("unable to implement manual placement request for population " << pop.id());
				size_t neurons = 0;
				size_t denmems = 0;
				for (auto const& request : queue) {
					auto const& slice = request.population_slice();
					neurons += slice.size();
					denmems += slice.size() * request.neuron_size();
					MAROCCO_DEBUG(
						"remaining " << slice << " with neuron size " << request.neuron_size());
				}
				MAROCCO_INFO(neurons << " neurons (" << denmems << " denmems) could not be placed");
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

void NeuronPlacement::run()
{
	if (m_parameters.minimize_number_of_sending_repeaters()) {
		minimize_number_of_sending_repeaters();
	}

	if (m_parameters.restrict_rightmost_neuron_blocks()) {
		restrict_rightmost_neuron_blocks();
	}

	auto auto_placements = perform_manual_placement();
	std::sort(
		auto_placements.begin(), auto_placements.end(),
		[&](NeuronPlacementRequest const& a, NeuronPlacementRequest const& b) -> bool {
			return m_graph[a.population()]->id() < m_graph[b.population()]->id();
		});

	std::vector<NeuronBlockOnWafer> neuron_blocks;
	neuron_blocks.reserve(m_denmem_assignment.size() * NeuronBlockOnHICANN::size);

	for (auto const& item : m_denmem_assignment) {
		for (auto const& nb : iter_all<NeuronBlockOnHICANN>()) {
			auto const& hicann = item.first;
			neuron_blocks.emplace_back(nb, hicann);
		}
	}

	PlacePopulations placer(m_denmem_assignment, neuron_blocks, auto_placements);
	auto const& result = placer.sort_and_run();
	post_process(result);

	if (!auto_placements.empty()) {
		throw ResourceExhaustedError("unable to place all populations");
	}

	// Denmem assignment is only stored for HICANNs that saw actual assignments.
	auto& dest = m_internal.denmem_assignment;
	for (auto const& hicann : m_used_hicanns) {
		dest[hicann] = m_denmem_assignment.at(hicann);
	}

	MAROCCO_INFO("Placement of populations finished");
}

void NeuronPlacement::post_process(std::vector<PlacePopulations::result_type> const& placements)
{
	for (auto const& primary_neuron : placements) {
		auto hicann = primary_neuron.toHICANNOnWafer();
		m_used_hicanns.insert(hicann);
		auto const& onb = m_denmem_assignment.at(hicann)[primary_neuron.toNeuronBlockOnHICANN()];
		auto it = onb.get(primary_neuron.toNeuronOnNeuronBlock());
		assert(it != onb.end() && *it != nullptr);
		auto const& population = (*it)->population();

		// Fill reverse lookup.
		m_internal.primary_denmems_for_population[population].push_back(primary_neuron);

		// Construct LogicalNeurons
		auto nb = primary_neuron.toNeuronBlockOnWafer();
		auto const& population_slice = (*it)->population_slice();
		size_t const hw_neuron_size = (*it)->neuron_size();
		for (auto& neuron : chunked(onb.neurons(it), hw_neuron_size)) {
			assert(!neuron.empty());
			NeuronOnNeuronBlock first = *neuron.begin();
			assert(first.y() == 0);
			size_t size = neuron.size();
			// Currently the placement fills from top→bottom, left→right (see
			// `internal::OnNeuronBlock`) thus creating block-shaped neurons where the
			// first row may contain one additional rightmost denmem (thus we round up below).
			auto logical_neuron = LogicalNeuron::on(nb)
				.add(NeuronOnNeuronBlock(first.x(), Y(0)), size - size / 2)
				.add(NeuronOnNeuronBlock(first.x(), Y(1)), size / 2)
				.done();
			BioNeuron bio_neuron(
			    population_slice.population(), population_slice.offset() + neuron.index());
			m_result.add(bio_neuron, logical_neuron);
		}
	}
}

} // namespace placement
} // namespace marocco
