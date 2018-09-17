#include "marocco/placement/algorithms/PlacePopulationsBase.h"

#include <boost/serialization/nvp.hpp>

#include "marocco/Logger.h"
#include "marocco/placement/internal/OnNeuronBlock.h"
#include "marocco/placement/internal/free_functions.h"

using namespace HMF::Coordinate;

namespace marocco {
namespace placement {
namespace algorithms {

boost::optional<std::vector<PlacePopulationsBase::result_type> > PlacePopulationsBase::run(
    graph_t const& graph,
    internal::Result::denmem_assignment_type& state,
    std::vector<HMF::Coordinate::NeuronBlockOnWafer>& neuron_blocks,
    std::vector<NeuronPlacementRequest>& queue)
{
	m_bio_graph = graph;
	m_state = state;
	m_neuron_blocks = neuron_blocks;
	m_queue = queue;
	m_result = std::vector<result_type>();


	if (m_queue->empty()) {
		// nothing to be placed. Return here so the user does not see following outputs, which might
		// unsettle the user.
		return m_result;
	}
	MAROCCO_INFO(
	    "Placing " << m_queue->size() << " population(s) on " << m_neuron_blocks->size()
	               << " NeuronBlocks");
	m_result->reserve(m_queue->size());

	MAROCCO_TRACE("init:");
	initialise(); // this shall be used as hook by derived classes

	MAROCCO_TRACE("place_one_pop:");
	while (place_one_population()) { // one hook is hidden in place_one_population after a
		                             // successful placement.
		MAROCCO_TRACE("loop:");
		loop(); // this shall be used as hook by derived classes
	};

	MAROCCO_TRACE("finalise:");
	finalise(); // this shall be used as hook by derived classes

	MAROCCO_DEBUG("placement finished with " << m_queue->size() << " PlacementRequests remaining");

	return m_result;
}

bool PlacePopulationsBase::place_one_population()
{
	if (m_queue->empty() || m_neuron_blocks->empty()) {
		return false;
	}

	// acquire the last neuron block, the loop() hook shall handle sorting to fulfill users wishes.
	auto nb = m_neuron_blocks->back();
	OnNeuronBlock& onb = internal::get_on_neuron_block_reference(*m_state, nb);

	// acquire the last placement request, the loop() hook shall handle sorting to fulfill users
	// wishes.
	NeuronPlacementRequest& placement = m_queue->back();
	auto& population = placement.population_slice();
	MAROCCO_DEBUG(
	    "Placing " << *((*m_bio_graph)[population.population()]) << " on neuron block " << nb
	               << ". Still available neurons " << onb.available());

	size_t const available =
	    onb.available() /
	    placement.neuron_size(); // this is intended as a flooring integer devision

	if (!available) {
		// This happens during manual placement, if different
		// populations have been placed to same HICANN.
		m_neuron_blocks->pop_back();
		return place_one_population();
	}

	auto chunk = NeuronPlacementRequest{population.slice_front(available), placement.neuron_size()};
	if (population.empty()) {
		m_queue->pop_back();
	}

	// Try to find a sufficiently large location.
	// This might fail if not enough consecutive space is left due to hardware defects.
	// TODO: Given defects this will likely fail every time, unless they are at the border?
	// NOTE by FP: a placement request can be a full population, populations can be larger than
	// neuron Blocks. Thus it will fail most of the time, until they are split to smaller parts.
	auto const it = onb.add(chunk);
	if (it != onb.end()) {
		NeuronOnNeuronBlock nrn = *(onb.neurons(it).begin());
		m_result->push_back(nrn.toNeuronOnWafer(nb));
		update_relations_to_placement(chunk, nb); // this a hook, for derived classes.
	} else if (chunk.population_slice().size() > 1) {
		// Split assignment and reinsert it.
		auto const parts = chunk.population_slice().split();
		for (auto const& pop : parts) {
			m_queue->push_back(NeuronPlacementRequest{pop, chunk.neuron_size()});
		}
	} else {
		// If size is already 1, terminal is useless.
		m_neuron_blocks->pop_back();
		m_queue->push_back(chunk);
	}
	MAROCCO_TRACE("result size: " << m_result->size());

	return true;
}

template <typename Archive>
void PlacePopulationsBase::serialize(Archive& /* ar */, unsigned int const /* version */)
{
	// no full serialisation possible, but the function is required by some classes.
	// when the commented datatypes are serializable, serialization should be enabled.
	// e.g. marocco/pymarocco/pymarocco_runtime/pymarocco_runtime.cpp:368:144:   required from here
	// and marocco/marocco/placement/parameters/NeuronPlacement.cpp:86
}
} // namespace algorithms
} // namespace placement
} // namespace marocco
BOOST_CLASS_EXPORT_IMPLEMENT(::marocco::placement::algorithms::PlacePopulationsBase)
#include "boost/serialization/serialization_helper.tcc"
EXPLICIT_INSTANTIATE_BOOST_SERIALIZE(::marocco::placement::algorithms::PlacePopulationsBase)
