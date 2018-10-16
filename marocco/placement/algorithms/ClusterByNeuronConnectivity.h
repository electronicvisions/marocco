#pragma once

#include <boost/unordered_map.hpp>

#include "marocco/placement/algorithms/ClusterByPopulationConnectivity.h"

#include "pywrap/compat/macros.hpp"
#include "marocco/coordinates/BioNeuron.h"

namespace marocco {
namespace placement {
namespace algorithms {

/** Placement of NeuronPlacementRequests to a list of NeuronBlocks.
 * Placement proceeds in a clustered way, such that single Neurons
 * that communicate with each other are located closer to each other.
 */
class ClusterByNeuronConnectivity : public ClusterByPopulationConnectivity
{
public:
	ClusterByNeuronConnectivity();

	/**
	 * these values are used to priorize in- and out-degrees.
	 *
	 * higher values mean a higher priority for the placement queue.
	 * the relative difference is relevant.
	 * T=1:S=1  all connections are equally weighted.
	 * T=1:S=10 sources have ten times the priroity -> a targeted neuron gets placed if ten times
	 * the connections compared to a source are already placed zero is a valid parameter, which will
	 * priorize one completly over the other.
	 */
	PYPP_INIT(size_t SortPriorityTargets, 1);
	PYPP_INIT(size_t SortPrioritySources, 1);

	/**
	 * @brief Control in which order neurons are added to the list of priorized placement requests
	 *
	 * target_and_source: add any neuron to the priority list that has a communication partner,
	 * that is already placed.
	 * target: only add targeted neurons
	 * source: only add sourcing neurons
	 * single_source: place all targeted neurons until priorized placements are empty, then add all
	 * targeted neurons of the last placement.
	 *
	 */
	// clang-format off
	PYPP_CLASS_ENUM(PlacementPriority)
	{
		target_and_source,
		target,
		source,
		single_source
	};
	// clang-format on
	PYPP_INIT(PlacementPriority m_population_placement_priority, PlacementPriority::target_and_source);

	std::string get_name() const PYPP_OVERRIDE;

	bool operator==(ClusterByNeuronConnectivity const& rhs) const;



protected:
	/**
	 * @brief every population is split into single neurons and stored in a "low priority" queue
	 **/
	void initialise() PYPP_OVERRIDE;

	/**
	 * @brief sorts the populations by the degree stored in m_precalculated_degree
	 **/
	void sort_population_priority() PYPP_OVERRIDE;

	/**
	 * @brief add new populations to the prio que and updates the degrees of pops related to placed
	 *pops
	 * @in NeuronPlacementRequest chunk: this NeuronPlacementRequest was placed, so we only need to
	 *search for relations to this one
	 **/
	virtual void update_population_priority_list(NeuronPlacementRequest const& chunk) PYPP_OVERRIDE;

	/**
	 * @brief add new neurons to the prio queue and update the degree of neurons related to placed
	 * populations.
	 *
	 * @param [in] BioNeuron placed_bio_nrn: This is BioNeuron was placed, we update relations to
	 * it.
	 */
	virtual void update_population_priority_list(BioNeuron const& placed_bio_nrn);

	/**
	 * @brief calculates the arithmetric mean of communication partners for the last request in
	 *m_queue.
	 **/
	virtual std::pair<double, double> center_of_partners() const PYPP_OVERRIDE;

	/**
	 * @brief handle the new placement, to be used later.
	 *        • stores the location of the PopulationSlice
	 *        • stores what populations were placed
	 *        • calls: calculation of the degree to related populations
	 *        • calls: update of the priority list in relation to the just placed population
	 * @param [in] NeuronPlacementRequest chunk: the PlacementRequest that was just placed.
	 * @param [in] HMF::Coordinate::NeuronBlockOnWafer nb: the NeuronBlock that was used for the
	 *placement.
	 *
	 **/
	void update_relations_to_placement(
	    NeuronPlacementRequest const& chunk,
	    HMF::Coordinate::NeuronBlockOnWafer const& nb) PYPP_OVERRIDE;


	/**
	 * @brief calculates the degree of a given BioNeuron to other Neurons placed.
	 * it uses the m_placed_mset multiset to have quick access to placed populaitons.
	 * @param [in] const graph_t::vertex_descriptor & pop: The population to calculate the degree
	 *for
	 * @return size_t: the degree
	 **/
	virtual size_t degree_to_placed(BioNeuron const& pop) const;

	/**
	 * @brief overloaded version of @degree_to_placed. it converts a NeuronPlacmentRequest to the
	 *populations vertex_descriptor
	 * @param [in] const NeuronPlacementRequest & req: The population to calculate the degree for
	 * @return size_t: the degree
	 **/
	size_t degree_to_placed(NeuronPlacementRequest const& req) const PYPP_OVERRIDE;

	/**
	 * @brief return the bio Nouron of a population slico of size 1
	 **/
	virtual const BioNeuron toBioNeuron(NeuronPlacementRequest const& npr) const;

	/**
	 * @brief returns true if bio_src has a synapse with non zero weight to bio_tgt
	 **/
	virtual bool is_connected(BioNeuron const& bio_src, BioNeuron const& bio_tgt) const;

	/**
	 * @brief creates a vector of Bio Neurons sourcing from the requested neuron
	 *
	 * @param [in] BioNeuron bio: calculates targets in reference to this Neuron
	 **/
	std::vector<BioNeuron> getTargetNeurons(BioNeuron const& bio) const;
	// a cache is used in the above function
	mutable boost::unordered_map<BioNeuron, std::vector<BioNeuron> > m_cached_targets;

	/**
	 * @brief creates a vector of Bio Neurons targeting the given neuron
	 *
	 * @param [in] BioNeuron bio: calculates the sources of the this Neuron
	 **/
	std::vector<BioNeuron> getSourceNeurons(BioNeuron const& bio) const;
	// a cache is used in the above function
	mutable boost::unordered_map<BioNeuron, std::vector<BioNeuron> > m_cached_sources;

	boost::unordered_map<BioNeuron, size_t> m_precalculated_degree;
	boost::unordered_map<BioNeuron, HMF::Coordinate::NeuronBlockOnWafer> m_placed;

	// history of placed neurons, used for some placement strategies
	std::vector<BioNeuron> m_placed_hist;
}; // ClusterByNeuronConnectivity

} // namespace internal
} // namespace placement
} // namespace marocco
