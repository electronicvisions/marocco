#include "marocco/placement/PlacePopulations.h"

#include "marocco/Logger.h"
#include "marocco/placement/OnNeuronBlock.h"
#include "marocco/placement/Result.h"
#include "marocco/util/spiral_ordering.h"

using namespace HMF::Coordinate;

namespace marocco {
namespace placement {

auto PlacePopulations::run() -> std::vector<result_type> const&
{
	MAROCCO_INFO("Placing " << m_queue.size() << " population(s)");
	m_result.reserve(m_queue.size());

	while (place_one_population()) {
		continue;
	};

	return m_result;
}

auto PlacePopulations::sort_and_run() -> std::vector<result_type> const&
{
	// TODO: As neuron blocks are sorted by available space, we should maybe also sort the
	// population queue by population size to prevent fragmentation of the populations.
	// (Populations are sliced up if they don't fit into a single neuron block.)
	// At the moment auto-placed populations are sorted by their id in HICANNPlacement::run().

	sort_neuron_blocks();

	return run();
}

void PlacePopulations::sort_neuron_blocks()
{
	MAROCCO_INFO("Sorting neuron blocks");

	std::unordered_map<NeuronBlockOnWafer, size_t> available;
	for (auto const& nb : m_neuron_blocks) {
		available.emplace(nb, on_neuron_block(nb).available());
	}

	// Because pop_back() is more efficient for vectors, neuron blocks are sorted by size
	// in descending order but nevertheless processed small-to-big.
	// If neuron blocks on the same HICANN have the same size, they shall be processed from left to
	// right. Therefore, these neuron blocks are sorted from right to left.
	const spiral_ordering<HICANNOnWafer> ordering;
	std::sort(
		m_neuron_blocks.begin(), m_neuron_blocks.end(),
		[&available, &ordering](NeuronBlockOnWafer const& a, NeuronBlockOnWafer const& b) {
			return (
				(available[a] > available[b]) ||
				(available[a] == available[b] &&
				 ((a.toHICANNOnWafer() == b.toHICANNOnWafer())
					  ? (a.toNeuronBlockOnHICANN() > b.toNeuronBlockOnHICANN())
					  : ordering(a.toHICANNOnWafer(), b.toHICANNOnWafer()))));
		});
}

OnNeuronBlock& PlacePopulations::on_neuron_block(NeuronBlockOnWafer const& nb)
{
	return m_state[nb.toHICANNOnWafer()][nb.toNeuronBlockOnHICANN()];
}

bool PlacePopulations::place_one_population()
{
	if (m_queue.empty() || m_neuron_blocks.empty()) {
		return false;
	}

	// We use the smallest possible neuron block, so we won't fragment the wafer too much.
	auto nb = m_neuron_blocks.back();
	auto hicann = nb.toHICANNOnWafer();
	OnNeuronBlock& onb = on_neuron_block(nb);

	NeuronPlacementRequest& placement = m_queue.back();
	size_t const available = onb.available() / placement.neuron_size();

	if (!available) {
		// This happens during manual placement, if different
		// populations have been placed to same hicann.
		m_neuron_blocks.pop_back();
		return place_one_population();
	}

	auto& population = placement.population_slice();
	auto chunk = NeuronPlacementRequest{population.slice_front(available), placement.neuron_size()};
	if (population.empty()) {
		m_queue.pop_back();
	}

	// Try to find a sufficiently large location.
	// This might fail if not enough consecutive space is left due to hardware defects.
	// TODO: Given defects this will likely fail every time, unless they are at the border?
	auto const it = onb.add(chunk);
	if (it != onb.end()) {
		NeuronOnNeuronBlock nrn = *(onb.neurons(it).begin());
		m_result.push_back(nrn.toNeuronOnWafer(nb));
	} else if (chunk.population_slice().size() > 1) {
		// Split assignment and reinsert it.
		auto const parts = chunk.population_slice().split();
		for (auto const& pop : parts) {
			m_queue.push_back(NeuronPlacementRequest{pop, chunk.neuron_size()});
		}
	} else {
		// If size is already 1, terminal is useless.
		m_neuron_blocks.pop_back();
		m_queue.push_back(chunk);
	}

	return true;
}

} // namespace placement
} // namespace marocco
