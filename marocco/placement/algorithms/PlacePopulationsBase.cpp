#include "marocco/placement/algorithms/PlacePopulationsBase.h"

#include <boost/serialization/nvp.hpp>

#include "marocco/Logger.h"
#include "marocco/placement/internal/OnNeuronBlock.h"
#include "marocco/placement/internal/free_functions.h"

using namespace halco::hicann::v2;

namespace marocco {
namespace placement {
namespace algorithms {

boost::optional<std::vector<PlacePopulationsBase::result_type> > PlacePopulationsBase::run(
    graph_t const& graph,
    internal::Result::denmem_assignment_type& state,
    std::vector<halco::hicann::v2::NeuronBlockOnWafer>& neuron_blocks,
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

bool PlacePopulationsBase::operator==(PlacePopulationsBase const& rhs) const
{
	bool ret = (typeid(*this) == typeid(rhs));

	// ret &= this->m_bio_graph == rhs.m_bio_graph; // as of boost 1.71, adjacency_list does not
	// have operator== https://www.boost.org/doc/libs/1_71_0/libs/graph/doc/adjacency_list.html
	// populations and edges are compared
	if (this->m_bio_graph != boost::none && rhs.m_bio_graph != boost::none) {
		auto this_vertices = boost::vertices(*(this->m_bio_graph));
		auto rhs_vertices = boost::vertices(*(rhs.m_bio_graph));
		if (std::distance(this_vertices.first, this_vertices.second) !=
		    std::distance(rhs_vertices.first, rhs_vertices.second)) {
			return false;
		}
		auto t_vit = this_vertices.first;
		auto r_vit = rhs_vertices.first;
		while (t_vit != this_vertices.second && r_vit != rhs_vertices.second) {
			ret &= *t_vit == *r_vit;
			t_vit++;
			r_vit++;
		}


		auto this_edges = boost::edges(*(this->m_bio_graph));
		auto rhs_edges = boost::edges(*(rhs.m_bio_graph));
		if (std::distance(this_edges.first, this_edges.second) !=
		    std::distance(rhs_edges.first, rhs_edges.second)) {
			return false;
		}
		auto t_eit = this_edges.first;
		auto r_eit = rhs_edges.first;
		while (t_eit != this_edges.second && t_eit != rhs_edges.second) {
			ret &= *t_eit == *r_eit;
			t_eit++;
			r_eit++;
		}
	} else {
		if (this->m_bio_graph != boost::none || rhs.m_bio_graph != boost::none) {
			// only one of them is none, so they are unequal
			return false;
		}
	}

	ret &= this->m_result == rhs.m_result;
	ret &= this->m_state == rhs.m_state;
	ret &= this->m_neuron_blocks == rhs.m_neuron_blocks;
	ret &= this->m_queue == rhs.m_queue;

	return ret;
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
