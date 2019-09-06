#include "marocco/placement/algorithms/ClusterByNeuronConnectivity.h"

#include <algorithm>

#include "hal/Coordinate/iter_all.h"
#include "marocco/BioGraph.h"
#include "marocco/Logger.h"
#include "marocco/coordinates/BioNeuron.h"
#include "marocco/placement/internal/free_functions.h"
#include "marocco/routing/util.h"
#include "marocco/util/spiral_ordering.h"
#include "marocco/util/vertical_ordering.h"

namespace marocco {
namespace placement {
namespace algorithms {

ClusterByNeuronConnectivity::ClusterByNeuronConnectivity() {}

void ClusterByNeuronConnectivity::initialise()
{
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

	auto request_it = m_queue->begin();
	MAROCCO_TRACE("splitting populations into single neurons");
	while (request_it != m_queue->end()) {
		if (request_it->population_slice().size() > 1) {
			size_t neuron_size = request_it->neuron_size();
			auto const& popSlice = request_it->population_slice().slice_front(1);

			m_queue->push_back(NeuronPlacementRequest(popSlice, neuron_size));
			request_it = m_queue->begin();
		} else {
			request_it++;
		}
	}

	m_unconnected = std::move(*m_queue);

	// if we are called after manual placement, move the connected neurons to the prioriy queue
	for (auto const& nrn : m_placed){
		update_population_priority_list(nrn.first);
	}


	MAROCCO_TRACE("first sort of pops");
	sort_population_priority();

	if (m_queue->empty()) {
		ClusterByPopulationConnectivity::add_first_population_to_priority_list();
	}

	MAROCCO_TRACE("first sort of neuron blocks");
	sort_neuron_blocks();
}

void ClusterByNeuronConnectivity::sort_population_priority()
{
	// a helper function: it is given to std::sort, to access the precalculated degrees
	auto degree_comp = [this](
	                       NeuronPlacementRequest const& a,
	                       NeuronPlacementRequest const& b) constexpr->bool
	{
		return this->m_precalculated_degree[toBioNeuron(a)] <
		       this->m_precalculated_degree[toBioNeuron(b)];
	};

	std::stable_sort(m_queue->begin(), m_queue->end(), degree_comp);
}

size_t ClusterByNeuronConnectivity::degree_to_placed(NeuronPlacementRequest const& req) const
{
	// helper function to determine the degree of req to the placed pops
	BioNeuron const bio = toBioNeuron(req);
	return degree_to_placed(bio);
}

size_t ClusterByNeuronConnectivity::degree_to_placed(const BioNeuron& bio) const
{
	size_t degree_target = 0;
	size_t degree_source = 0;

	auto const& nrn_targets = getTargetNeurons(bio);
	auto const& nrn_sources = getSourceNeurons(bio);

	if (!nrn_targets.empty()) {
		for (auto const& bio_target : nrn_targets) {
			degree_target += m_placed.count(bio_target);
		}
	}

	if (!nrn_sources.empty()) {
		for (auto const& bio_source : nrn_sources) {
			degree_source += m_placed.count(bio_source);
		}
	}

	// extra weight to sources
	degree_source *= SortPrioritySources;
	// extra weight to targets
	degree_target *= SortPriorityTargets;

	return degree_target + degree_source;
}


bool ClusterByNeuronConnectivity::is_connected(
    const BioNeuron& bio_src, const BioNeuron& bio_tgt) const
{
	auto const& outs = make_iterable(out_edges(bio_src.population(), *m_bio_graph));

	for (auto const& edge : outs) {
		if (bio_tgt.population() != boost::target(edge, *m_bio_graph)) {
			// the target neurons population is not the target of this edge
			continue;
		}

		ProjectionView const proj_view = (*m_bio_graph)[edge]; // get the projection for this edge
		if (!proj_view.pre().mask()[bio_src.neuron_index()]) {
			// does the source bio neuron want to realise a connection on this edge???
			continue;
		}

		if (!proj_view.post().mask()[bio_tgt.neuron_index()]) {
			// does the target bio neuron has a connection to realise this edge???
			continue;
		}

		Connector::const_matrix_view_type const bio_weights = proj_view.getWeights();

		size_t const src_neuron_in_proj_view =
		    routing::to_relative_index(proj_view.pre().mask(), bio_src.neuron_index());
		size_t const trg_neuron_in_proj_view =
		    routing::to_relative_index(proj_view.post().mask(), bio_tgt.neuron_index());


		double const weight = bio_weights(src_neuron_in_proj_view, trg_neuron_in_proj_view);

		if (std::isnan(weight) || weight <= 0.) {
			// is there a weight to realise
			continue;
		}

		// all checks passed;
		return true;
	}
	return false;
}

std::vector<BioNeuron> ClusterByNeuronConnectivity::getTargetNeurons(BioNeuron const& bio) const
{
	auto search = m_cached_targets.find(bio);
	if (search == m_cached_targets.end()) {
		std::vector<BioNeuron> result;

		auto const& edges_out = make_iterable(out_edges(bio.population(), *m_bio_graph));

		for (auto const& edge : edges_out) {
			graph_t::vertex_descriptor target_vertex = boost::target(edge, *m_bio_graph);
			Population const& target_pop = *((*m_bio_graph)[target_vertex]);

			for (size_t n = 0; n < target_pop.size(); ++n) {
				BioNeuron const bio_tgt = BioNeuron(target_vertex, n);
				if (is_connected(bio, bio_tgt)) {
					result.push_back(bio_tgt);
				}
			}
		}
		std::tie(search, std::ignore) = m_cached_targets.emplace(bio, result);
	}
	return (*search).second;
}

std::vector<BioNeuron> ClusterByNeuronConnectivity::getSourceNeurons(BioNeuron const& bio) const
{
	auto search = m_cached_sources.find(bio);
	if (search == m_cached_sources.end()) {
		std::vector<BioNeuron> result;

		auto const& edges_in = make_iterable(in_edges(bio.population(), *m_bio_graph));

		for (auto const& edge : edges_in) {
			graph_t::vertex_descriptor source_vertex = boost::source(edge, *m_bio_graph);
			Population const& source_pop = *((*m_bio_graph)[source_vertex]);

			for (size_t n = 0; n < source_pop.size(); ++n) {
				BioNeuron bio_src = BioNeuron(source_vertex, n);
				if (is_connected(bio_src, bio)) {
					result.push_back(bio_src);
				}
			}
		}
		std::tie(search, std::ignore) = m_cached_sources.emplace(bio, result);
	}
	return (*search).second;
}

void ClusterByNeuronConnectivity::update_population_priority_list(
    NeuronPlacementRequest const& chunk)
{
	BioNeuron const placed_bio_nrn = toBioNeuron(chunk);
	update_population_priority_list(placed_bio_nrn);
}

void ClusterByNeuronConnectivity::update_population_priority_list(BioNeuron const& placed_bio_nrn)
{
	// add new Neurons to the priority queue.
	// they have to be connected to already placed Neurons
	// chunk is the populationSlice/aka. Neuron that was placed, so we only need its partners

	// precalculate the degree for all target and source populations, and store the degree in a map.
	// this saves lot of computation in the sorting process.

	MAROCCO_TRACE(
	    "updating prio list"
	    << "\n prio " << m_queue->size() << "\n placed " << m_placed.size() << "\n queue "
	    << m_unconnected.size());
	MAROCCO_TRACE("adding neighbours of bio neuron" << placed_bio_nrn);

	auto const& nrn_targets = getTargetNeurons(placed_bio_nrn);
	auto const& nrn_sources = getSourceNeurons(placed_bio_nrn);

	if (!nrn_targets.empty()) {
		for (auto const& bio_target : nrn_targets) {
			// precalculate the degree and save it
			// the target has us (the placed) as source, add the configurable SortPrioritySources
			m_precalculated_degree[bio_target] += SortPrioritySources;

			// only add to prio_que if desired
			if (m_population_placement_priority == PlacementPriority::target_and_source ||
			    m_population_placement_priority == PlacementPriority::target) {
				for (auto itq = m_unconnected.begin(); itq != m_unconnected.end(); itq++) {
					if (toBioNeuron(*itq) == bio_target) {
						m_queue->push_back(*itq);
						itq = m_unconnected.erase(itq);
						itq--;
					}
				}
			}
		}
	}

	if (!nrn_sources.empty()) {
		for (auto const& bio_source : nrn_sources) {
			// SortPriorityTargets can be easily configured by a user
			m_precalculated_degree[bio_source] += SortPriorityTargets;

			// only add to the prio_que if it is desired
			if (m_population_placement_priority == PlacementPriority::target_and_source ||
			    m_population_placement_priority == PlacementPriority::source) {
				for (auto itq = m_unconnected.begin(); itq != m_unconnected.end(); itq++) {
					if (toBioNeuron(*itq) == bio_source) {
						m_queue->push_back(*itq);
						itq = m_unconnected.erase(itq);
						itq--;
					}
				}
			}
		}
	}

	if (m_queue->empty()) {
		if (m_population_placement_priority == PlacementPriority::single_source) {
			while (!m_placed_hist.empty()) {
				auto const& nrn = m_placed_hist.back();
				m_placed_hist.pop_back();
				auto const& targets = getTargetNeurons(nrn);
				for (auto const& target : targets) {
					for (auto itq = m_unconnected.begin(); itq != m_unconnected.end(); itq++) {
						if (toBioNeuron(*itq) == target) {
							m_queue->push_back(*itq);
							itq = m_unconnected.erase(itq);
							itq--;
						}
					}
				}
				if (!m_queue->empty()) {
					// we now have Neurons to place, thus leave the loop over the placed neurons
					return;
				}
			}
		}
	}
}

const BioNeuron ClusterByNeuronConnectivity::toBioNeuron(const NeuronPlacementRequest& npr) const
{
	if (npr.population_slice().size() != 1) {
		MAROCCO_ERROR(
		    "only request BioNeuron from PlacementRequest if it is of size 1, actual size: "
		    << npr.population_slice().size());
		throw std::runtime_error(
		    "somehow a placement request with more than 1 bio neuron ended up here");
	}


	return BioNeuron(npr.population(), npr.population_slice().offset());
}

std::pair<double, double> ClusterByNeuronConnectivity::center_of_partners() const
{
	double x = HMF::Coordinate::HICANNOnWafer::x_type::max / 2.;
	double y = HMF::Coordinate::HICANNOnWafer::y_type::max / 2.;

	if (m_queue->empty()) {
		return std::pair<double, double>(x, y);
	}
	auto const& next_chunk = m_queue->back();
	auto const& nrn_next = toBioNeuron(next_chunk);
	size_t x_counter = 0, y_counter = 0;
	size_t avg_counter = 0;

	if (m_spiral_center == SpiralCenter::spiral_neighbours_target ||
	    m_spiral_center == SpiralCenter::spiral_neighbours) {
		auto const& partners = getTargetNeurons(nrn_next);
		if (!partners.empty()) {
			for (auto itp = m_placed.begin(); itp != m_placed.end(); itp++) {
				BioNeuron const placedNeuron = itp->first;
				if (std::find(partners.begin(), partners.end(), placedNeuron) != partners.end()) {
					x_counter += itp->second.toHICANNOnWafer().x();
					y_counter += itp->second.toHICANNOnWafer().y();
					avg_counter++;
				}
			}
		}
	}

	if (m_spiral_center == SpiralCenter::spiral_neighbours_source ||
	    m_spiral_center == SpiralCenter::spiral_neighbours) {
		auto const& partners = getSourceNeurons(nrn_next);
		if (!partners.empty()) {
			for (auto itp = m_placed.begin(); itp != m_placed.end(); itp++) {
				BioNeuron const placedNeuron = itp->first;
				if (std::find(partners.begin(), partners.end(), placedNeuron) != partners.end()) {
					x_counter += itp->second.toHICANNOnWafer().x();
					y_counter += itp->second.toHICANNOnWafer().y();
					avg_counter++;
				}
			}
		}
	}

	if (avg_counter != 0) {
		MAROCCO_TRACE(x_counter << " x|y " << y_counter << " connections: " << avg_counter);
		x = static_cast<double>(x_counter) / static_cast<double>(avg_counter);
		y = static_cast<double>(y_counter) / static_cast<double>(avg_counter);
	}
	return std::pair<double, double>(x, y);
}

void ClusterByNeuronConnectivity::update_relations_to_placement(
    NeuronPlacementRequest const& chunk, HMF::Coordinate::NeuronBlockOnWafer const& nb)
{
	MAROCCO_TRACE(" update_relations_to_placement()");
	// add the location of the population to a map, so clustering can be done, difficult to search
	// for populations
	m_placed.emplace(toBioNeuron(chunk), nb);

	m_placed_hist.push_back(toBioNeuron(chunk));

	// add new populations to the prio que, update the degrees of pops to placed pops
	update_population_priority_list(chunk);

	// in cases where nothing is connected anymore, a new starting point hast to be found
	if(m_queue->empty()){
		ClusterByPopulationConnectivity::add_first_population_to_priority_list();
	}

}

bool ClusterByNeuronConnectivity::operator==(ClusterByNeuronConnectivity const& rhs) const
{
	bool ret = true;
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
	ret &= this->m_cached_targets == rhs.m_cached_targets;
	ret &= this->m_cached_sources == rhs.m_cached_sources;
	ret &= this->m_precalculated_degree == rhs.m_precalculated_degree;
	ret &= this->m_placed_hist == rhs.m_placed_hist;
	return ret;
}

} // namespace internal
} // namespace placement
} // namespace marocco
