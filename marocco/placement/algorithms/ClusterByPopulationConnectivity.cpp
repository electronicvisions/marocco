#include "marocco/placement/algorithms/ClusterByPopulationConnectivity.h"

#include <algorithm>
#include <functional>
#include <utility>

#include "hal/Coordinate/iter_all.h"
#include "marocco/BioGraph.h"
#include "marocco/Logger.h"
#include "marocco/placement/internal/free_functions.h"
#include "marocco/util/spiral_ordering.h"
#include "marocco/util/vertical_ordering.h"

namespace marocco {
namespace placement {
namespace algorithms {

ClusterByPopulationConnectivity::ClusterByPopulationConnectivity() {}

void ClusterByPopulationConnectivity::initialise()
{
	MAROCCO_INFO("using placement strategy: ClusterByPopulationConnectivity");
	for (auto nb_it = m_neuron_blocks->begin(); nb_it != m_neuron_blocks->end();) {
		auto const& hicann = nb_it->toHICANNOnWafer();
		auto it = m_state->find(hicann);
		if (it == m_state->end()) {
			nb_it = m_neuron_blocks->erase(nb_it);
			MAROCCO_DEBUG(hicann << " unavailable during manual population placement, removing it");
		} else {
			nb_it++;
		}
	}
	m_unconnected.clear();
	m_unconnected = std::move(*m_queue); // at first all populations are unconnected

	// if there are placements from a prior manual placement request, the queue has to be updated
	for (auto const placement : m_placed) {
		update_population_priority_list(placement.first);
	}

	MAROCCO_TRACE("first sort of pops");
	sort_population_priority();

	if (m_queue->empty()) {
		add_first_population_to_priority_list();
	}

	MAROCCO_TRACE("first sort of neuron blocks");
	sort_neuron_blocks();
}

void ClusterByPopulationConnectivity::loop()
{
	// sort the populations in the prio queue by their degree to placed pops
	MAROCCO_TRACE("now sorting pops");
	sort_population_priority();

	// add new populations to the priority queue, by degree to placed pops
	// update_population_priority_list();
	if (m_queue->empty()) {
		add_first_population_to_priority_list();
	}

	// the order of neuron blocks is determined by sort_neuron_blocks()
	MAROCCO_TRACE("now sorting neuron blocks");
	sort_neuron_blocks();
}

void ClusterByPopulationConnectivity::finalise()
{
	// unplaced pops are put back to normal queue
	for (auto elem : m_unconnected) {
		m_queue->push_back(elem);
	}
}

bool ClusterByPopulationConnectivity::nb_order_function(
    HMF::Coordinate::NeuronBlockOnWafer const& a,
    HMF::Coordinate::NeuronBlockOnWafer const& b,
    std::function<bool(
        HMF::Coordinate::NeuronBlockOnWafer const&,
        HMF::Coordinate::NeuronBlockOnWafer const&)> const& order) const
{
	if (a == b)
		return false;
	if (order(a, b) == order(b, a)) {
		return a.toEnum() < b.toEnum();
	}
	return order(a, b);
}

bool ClusterByPopulationConnectivity::hicann_order_function(
    HMF::Coordinate::HICANNOnWafer const& a,
    HMF::Coordinate::HICANNOnWafer const& b,
    std::function<bool(
        HMF::Coordinate::HICANNOnWafer const&, HMF::Coordinate::HICANNOnWafer const&)> const& order)
    const
{
	if (a == b)
		return false;
	if (order(a, b) == order(b, a)) {
		return a.toEnum() < b.toEnum();
	}
	return order(a, b);
}

bool ClusterByPopulationConnectivity::neuron_blocks_comparator_function(
    HMF::Coordinate::NeuronBlockOnWafer const& a,
    HMF::Coordinate::NeuronBlockOnWafer const& b,
    std::function<
        bool(HMF::Coordinate::HICANNOnWafer const&, HMF::Coordinate::HICANNOnWafer const&)> const&
        hicann_order,
    std::function<bool(
        HMF::Coordinate::NeuronBlockOnWafer const&,
        HMF::Coordinate::NeuronBlockOnWafer const&)> const& nb_order) const
{
	if (a == b)
		return false;
	switch (m_neuron_block_on_wafer_ordering) {
		case NeuronBlockOnWaferOrdering::neuron_block_on_hicann_then_hicann_on_wafer: {
			return !(
			    a.toNeuronBlockOnHICANN() == b.toNeuronBlockOnHICANN()
			        ? hicann_order(a.toHICANNOnWafer(), b.toHICANNOnWafer())
			        : nb_order(
			              a, b)); // negated, because vector operations are performed on the back
		}
		case NeuronBlockOnWaferOrdering::hicann_on_wafer_then_neuron_block_on_hicann: {
			return !(
			    a.toHICANNOnWafer() == b.toHICANNOnWafer()
			        ? nb_order(a, b)
			        : hicann_order(a.toHICANNOnWafer(), b.toHICANNOnWafer()));
		}
		default: {
			MAROCCO_ERROR(
			    "automatic placement ordering ordering "
			    << static_cast<size_t>(m_neuron_block_on_wafer_ordering) << " unknown");
			throw std::runtime_error("automatic placement unknown neuron_block_on_wafer_ordering");
		}
	}
}

void ClusterByPopulationConnectivity::sort_neuron_blocks()
{
	std::function<bool(
	    HMF::Coordinate::NeuronBlockOnWafer const&, HMF::Coordinate::NeuronBlockOnWafer const&)>
	    nb_ordering;

	switch (m_neuron_block_on_hicann_ordering) {
		case NeuronBlockOnHICANNOrdering::neuron_block_on_hicann_enum_increasing: {
			MAROCCO_TRACE("strat:increasing");
			nb_ordering = [](HMF::Coordinate::NeuronBlockOnWafer const& a,
			                 HMF::Coordinate::NeuronBlockOnWafer const& b) constexpr->bool
			{
				return a.toNeuronBlockOnHICANN() < b.toNeuronBlockOnHICANN();
			};
			break;
		}
		case NeuronBlockOnHICANNOrdering::neuron_block_on_hicann_enum_decreasing: {
			MAROCCO_TRACE("strat:decreasing");
			nb_ordering = [](HMF::Coordinate::NeuronBlockOnWafer const& a,
			                 HMF::Coordinate::NeuronBlockOnWafer const& b) constexpr->bool
			{
				return a.toNeuronBlockOnHICANN() > b.toNeuronBlockOnHICANN();
			};
			break;
		}
		case NeuronBlockOnHICANNOrdering::merger_tree_friendly: {
			MAROCCO_TRACE("strat:merger_friendly");
			nb_ordering = [](HMF::Coordinate::NeuronBlockOnWafer const& a,
			                 HMF::Coordinate::NeuronBlockOnWafer const& b) constexpr->bool
			{
				// 3 2 1 0 5 4 6 7  NB
				// 0 1 2 3 4 5 6 7  Sorted to location
				const size_t mapping[] = {3, 2, 1, 0, 5, 4, 6, 7};
				return mapping[a.toNeuronBlockOnHICANN()] < mapping[b.toNeuronBlockOnHICANN()];
			};
			break;
		}
		default: {
			MAROCCO_ERROR(
			    "automatic placement neuron_block_on_hicann ordering "
			    << static_cast<size_t>(m_neuron_block_on_hicann_ordering) << " unknown");
			throw std::runtime_error("unknown neuron_block_on_hicann_ordering");
		}
	}


	auto nb_order = std::bind(
	    &marocco::placement::algorithms::ClusterByPopulationConnectivity::nb_order_function, this,
	    std::placeholders::_1, std::placeholders::_2, nb_ordering);

	auto [center_x, center_y] = center_of_partners();

	std::function<bool(
	    HMF::Coordinate::HICANNOnWafer const&, HMF::Coordinate::HICANNOnWafer const&)>
	    hc_ordering;

	switch (m_hicann_on_wafer_ordering) {
		case HICANNOnWaferOrdering::spiral: {
			spiral_ordering<HMF::Coordinate::HICANNOnWafer> spiral_order(center_x, center_y);
			hc_ordering = spiral_order;
			MAROCCO_TRACE("spiral order");
			break;
		}
		case HICANNOnWaferOrdering::vertical: {
			vertical_ordering<HMF::Coordinate::HICANNOnWafer> vertical_order(center_x, center_y);
			hc_ordering = vertical_order;
			MAROCCO_TRACE("vertical order");
			break;
		}
		default: {
			throw std::runtime_error("HICANN order not known");
		}
	}

	auto hicann_order = std::bind(
	    &marocco::placement::algorithms::ClusterByPopulationConnectivity::hicann_order_function,
	    this, std::placeholders::_1, std::placeholders::_2,
	    [&hc_ordering](
	        HMF::Coordinate::HICANNOnWafer const& a,
	        HMF::Coordinate::HICANNOnWafer const& b) constexpr->bool { return hc_ordering(a, b); });

	auto sort_neuron_blocks_comparator = std::bind(
	    &marocco::placement::algorithms::ClusterByPopulationConnectivity::
	        neuron_blocks_comparator_function,
	    this, std::placeholders::_1, std::placeholders::_2,
	    [&hicann_order](
	        HMF::Coordinate::HICANNOnWafer const& a,
	        HMF::Coordinate::HICANNOnWafer const& b) constexpr->bool { return hicann_order(a, b); },
	    [&nb_order](
	        HMF::Coordinate::NeuronBlockOnWafer const& a,
	        HMF::Coordinate::NeuronBlockOnWafer const& b) constexpr->bool {
		    return nb_order(a, b);
	    });

	MAROCCO_TRACE("sorting");
	std::sort(m_neuron_blocks->begin(), m_neuron_blocks->end(), sort_neuron_blocks_comparator);
	MAROCCO_TRACE("done sorting");
}

void ClusterByPopulationConnectivity::sort_population_priority()
{
	// a helper function: it is given to std::sort, to access the precalculated degrees
	auto degree_comp = [this](
	                       NeuronPlacementRequest const& a,
	                       NeuronPlacementRequest const& b) constexpr->bool
	{
		return this->m_precalculated_degree[a.population()] <
		       this->m_precalculated_degree[b.population()];
	};

	std::sort(m_queue->begin(), m_queue->end(), degree_comp);
}

size_t ClusterByPopulationConnectivity::degree_to_placed(NeuronPlacementRequest const& req) const
{
	// helper function to determine the degree of req to the placed pops
	auto const& pop = req.population();
	return degree_to_placed(pop);
}

size_t ClusterByPopulationConnectivity::degree_to_placed(
    graph_t::vertex_descriptor const& pop) const
{
	size_t degree_target = 0;
	size_t degree_source = 0;
	auto const& targets = adjacent_vertices(pop, *m_bio_graph);
	auto const& sources = inv_adjacent_vertices(pop, *m_bio_graph);

	for (auto its = sources.first; its != sources.second; its++) {
		degree_source += m_placed_mset[((*m_bio_graph)[*its])->id()];
	}

	for (auto itt = targets.first; itt != targets.second; itt++) {
		degree_target += m_placed_mset[((*m_bio_graph)[*itt])->id()];
	}

	// extra weight to sources
	degree_source *= SortPrioritySources;
	// extra weight to targets
	degree_target *= SortPriorityTargets;

	return degree_target + degree_source;
}


void ClusterByPopulationConnectivity::add_first_population_to_priority_list()
{
	// we have to place the first population, or all connected pops were placed, so we take a new
	// starting pop
	if (m_queue->empty()) {
		if (m_unconnected.empty()) {
			return;
		}
		MAROCCO_TRACE("adding an unconnected population to placement queue");
		MAROCCO_TRACE("degree     : " << degree(m_unconnected.back().population(), *m_bio_graph));
		MAROCCO_TRACE(
		    "degree in  : " << in_degree(m_unconnected.back().population(), *m_bio_graph));
		MAROCCO_TRACE(
		    "degree out : " << out_degree(m_unconnected.back().population(), *m_bio_graph));
		m_queue->push_back(m_unconnected.back());
		m_unconnected.pop_back();
		return; // we shall not add new pops adjecent to this new pop
	}
}

void ClusterByPopulationConnectivity::update_population_priority_list(
    NeuronPlacementRequest const& chunk)
{
	// add new populations to the priority queue.
	// they have to be connected to already placed populations
	// chunk is the population that was placed, so we only need its partners

	// Precalculate the degree for all target and source populations, and store the degree in a map.
	// this saves lot of computation in the sorting process.

	MAROCCO_TRACE("update pop prio list");

	MAROCCO_TRACE(
	    "updating placement queue"
	    << "\n waiting " << m_queue->size() << "\n placed " << m_placed.size() << "\n unconnected "
	    << m_unconnected.size());
	auto const& pop_p = std::pair(chunk, std::ignore);
	MAROCCO_TRACE("adding neighbours of pop :" << pop_p.first.population());
	auto const& targets = adjacent_vertices(pop_p.first.population(), *m_bio_graph);
	auto const& sources = inv_adjacent_vertices(pop_p.first.population(), *m_bio_graph);

	for (auto itt = targets.first; itt != targets.second; itt++) {
		// precalculate the degree and save it
		m_precalculated_degree[*itt] = degree_to_placed(*itt);

		// only add to prio_que if desired
		if (m_population_placement_priority == PopulationPlacementPriority::target_and_source ||
		    m_population_placement_priority == PopulationPlacementPriority::target) {
			for (auto itq = m_unconnected.begin(); itq != m_unconnected.end(); itq++) {
				if ((*m_bio_graph)[*itt] == (*m_bio_graph)[itq->population()]) {
					m_queue->push_back(*itq);
					itq = m_unconnected.erase(itq);
					itq--;
				}
			}
		}
	}

	for (auto its = sources.first; its != sources.second; its++) {
		// precalculate the degree of the sources and targets
		m_precalculated_degree[*its] = degree_to_placed(*its);

		// only add to the prio_que if it is desired
		if (m_population_placement_priority == PopulationPlacementPriority::target_and_source ||
		    m_population_placement_priority == PopulationPlacementPriority::source) {
			for (auto itq = m_unconnected.begin(); itq != m_unconnected.end(); itq++) {
				if ((*m_bio_graph)[*its] == (*m_bio_graph)[itq->population()]) {
					m_queue->push_back(*itq);
					itq = m_unconnected.erase(itq);
					itq--;
				}
			}
		}
	}
}

std::pair<double, double> ClusterByPopulationConnectivity::center_of_partners() const
{
	double x = HMF::Coordinate::HICANNOnWafer::x_type::max / 2.;
	double y = HMF::Coordinate::HICANNOnWafer::y_type::max / 2.;

	if (m_queue->empty()) {
		return std::pair<double, double>(x, y);
	}
	// avg_location_of_neighbours of front
	MAROCCO_TRACE("centre of communication partner calculation");
	auto const& pop_next = m_queue->back();
	auto const& targets = adjacent_vertices(pop_next.population(), *m_bio_graph);
	auto const& sources = inv_adjacent_vertices(pop_next.population(), *m_bio_graph);
	size_t x_counter = 0, y_counter = 0;
	size_t avg_counter = 0;

	if (m_spiral_center == SpiralCenter::spiral_neighbours_target ||
	    m_spiral_center == SpiralCenter::spiral_neighbours) {
		for (auto itt = targets.first; itt != targets.second; itt++) {
			for (auto itp = m_placed.begin(); itp != m_placed.end(); itp++) {
				if ((*m_bio_graph)[*itt] == (*m_bio_graph)[itp->first.population()]) {
					x_counter += itp->second.toHICANNOnWafer().x();
					y_counter += itp->second.toHICANNOnWafer().y();
					avg_counter++;
				}
			}
		}
	}
	if (m_spiral_center == SpiralCenter::spiral_neighbours_source ||
	    m_spiral_center == SpiralCenter::spiral_neighbours) {
		for (auto its = sources.first; its != sources.second; its++) {
			for (auto itp = m_placed.begin(); itp != m_placed.end(); itp++) {
				if ((*m_bio_graph)[*its] == (*m_bio_graph)[itp->first.population()]) {
					x_counter += itp->second.toHICANNOnWafer().x();
					y_counter += itp->second.toHICANNOnWafer().y();
					avg_counter++;
				}
			}
		}
	}

	if (avg_counter != 0) {
		MAROCCO_TRACE(x_counter << " x|y " << y_counter << " ctr: " << avg_counter);
		x = static_cast<double>(x_counter) / static_cast<double>(avg_counter);
		y = static_cast<double>(y_counter) / static_cast<double>(avg_counter);
	}
	return std::pair<double, double>(x, y);
}

void ClusterByPopulationConnectivity::update_relations_to_placement(
    NeuronPlacementRequest const& chunk, HMF::Coordinate::NeuronBlockOnWafer const& nb)
{
	MAROCCO_TRACE("update relations");
	// add the location of the population to a map, so clustering can be done, difficult to search
	// for populations
	m_placed.insert(std::pair(chunk, nb));

	// pre store the placed pops in multiset for easy access, just needed for analysis which
	// population has been placed, easy searchable when the population is known.
	m_placed_mset[(*m_bio_graph)[chunk.population()]->id()] += 1;

	// save the degree to other populations
	m_precalculated_degree[chunk.population()] = degree_to_placed(chunk.population());

	// add new populations from the unconnected que to the placement que and updates the degrees of
	// all connected pops to this placed pop
	update_population_priority_list(chunk);

	// if no placement request was added, but the queue is empty, add an element.
	if (m_queue->empty()) {
		add_first_population_to_priority_list();
	}
}

bool ClusterByPopulationConnectivity::operator==(ClusterByPopulationConnectivity const& rhs) const
{
	bool ret = true;
	MAROCCO_WARN("also here");
	ret &= this->PlacePopulationsBase::operator==(rhs);
	ret &= this->m_neuron_block_on_hicann_ordering == rhs.m_neuron_block_on_hicann_ordering;
	ret &= this->m_neuron_block_on_wafer_ordering == rhs.m_neuron_block_on_wafer_ordering;
	ret &= this->m_hicann_on_wafer_ordering == rhs.m_hicann_on_wafer_ordering;
	ret &= this->m_spiral_center == rhs.m_spiral_center;
	ret &= this->SortPriorityTargets == rhs.SortPriorityTargets;
	ret &= this->SortPrioritySources == rhs.SortPrioritySources;
	ret &= this->m_population_placement_priority == rhs.m_population_placement_priority;
	ret &= this->m_unconnected == rhs.m_unconnected;
	ret &= this->m_placed_mset == rhs.m_placed_mset;
	ret &= this->m_placed == rhs.m_placed;
	ret &= this->m_precalculated_degree == rhs.m_precalculated_degree;

	return ret;
}

} // namespace internal
} // namespace placement
} // namespace marocco
