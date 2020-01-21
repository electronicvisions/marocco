#pragma once

#include "marocco/placement/algorithms/PlacePopulationsBase.h"

namespace marocco {
namespace placement {
namespace algorithms {

/** Placement of populations to a list of neuron blocks.
 * Placement proceeds in a clustered way, such that populations
 * that communicate with each other are located closer to each other
 */
class ClusterByPopulationConnectivity : public PlacePopulationsBase
{
public:
	ClusterByPopulationConnectivity();

	/**
	 * @brief Control in which order NeuronBlockOnHICANN are picked during automatic placement
	 *
	 * neuron_block_on_hicann_enum_increasing: lower enums first
	 * neuron_block_on_hicann_enum_decreasing: higher enums first
	 * merger_tree_friendly: use at first NB 3 then the lower NBs (that can be merged to 3), then 5
	 * then 4, then 6 and 7
	 *
	 */
	// clang-format off
	PYPP_CLASS_ENUM(NeuronBlockOnHICANNOrdering)
	{
		neuron_block_on_hicann_enum_increasing,
		neuron_block_on_hicann_enum_decreasing,
		merger_tree_friendly
	};
	// clang-format on
	PYPP_INIT(
	    NeuronBlockOnHICANNOrdering m_neuron_block_on_hicann_ordering,
	    NeuronBlockOnHICANNOrdering::merger_tree_friendly);

	/**
	 * @brief Control in which order NeuronBlockOnWafer are picked during automatic placement
	 *
	 * hicann_on_wafer_then_neuron_block_on_hicann: sort by HICANNOnWafer first, then by
	 * NeuronBlockOnHICANN neuron_block_on_hicann_then_hicann_on_wafer: sort by NeuronBlockOnHICANN
	 * first, then by HICANNOnWafer
	 *
	 * Defaults to hicann_on_wafer_then_neuron_block_on_hicann
	 */
	// clang-format off
	PYPP_CLASS_ENUM(NeuronBlockOnWaferOrdering)
	{
		hicann_on_wafer_then_neuron_block_on_hicann,
		neuron_block_on_hicann_then_hicann_on_wafer
	};
	// clang-format on
	PYPP_INIT(
	    NeuronBlockOnWaferOrdering m_neuron_block_on_wafer_ordering,
	    NeuronBlockOnWaferOrdering::hicann_on_wafer_then_neuron_block_on_hicann);

	/**
	 * @brief controls the order of hicanns on wafer
	 *
	 * spiral: sort by a spiral around the average position
	 * vertical: use all y on a given x coordinate, then propagate to the next x. Starts at the
	 * average
	 */
	// clang-format off
	PYPP_CLASS_ENUM(HICANNOnWaferOrdering)
	{
		spiral,
		vertical
	};
	// clang-format on
	PYPP_INIT(HICANNOnWaferOrdering m_hicann_on_wafer_ordering, HICANNOnWaferOrdering::spiral);

	/**
	 * @brief Control which HICANNs are considered to calculate the center of the spiral used for
	 * HICANN ordering
	 *
	 * spiral_neighbours: center the spiral/vertical at already placed communicating populations
	 * spiral_neighbours_target: spiral/vertical close to targeted populations
	 * spiral_neighbours_source: spiral/vertical close to source populations
	 *
	 * Defaults to spiral_center_available_hicanns
	 */
	// clang-format off
	PYPP_CLASS_ENUM(SpiralCenter)
	{
		spiral_neighbours,
		spiral_neighbours_target,
		spiral_neighbours_source,
	};
	// clang-format on
	PYPP_INIT(SpiralCenter m_spiral_center, SpiralCenter::spiral_neighbours);

	/**
	 * @brief Control in which order populations are placed on ordered NeuronBlocks
	 *
	 * how important are connections to targets or connections to sources.
	 * if SortPriorityTargets is higher than SportPrioritySources, then populations where targets
	 * have already been placed are sorted to the front and will be placed early
	 */
	PYPP_INIT(size_t SortPriorityTargets, 1);
	PYPP_INIT(size_t SortPrioritySources, 1);

	/**
	 * @brief Control in which order populations are added to the list of populations that will be
	 * considered in the degree computation
	 *
	 * target_and_source: add any population to the priority list that has a communication partner,
	 * that is already placed.
	 * target: only add targeted populations to the priority list
	 * source: add sources of a just placed population to the priority list.
	 *
	 */
	// clang-format off
	PYPP_CLASS_ENUM(PopulationPlacementPriority)
	{
		target_and_source,
		target,
		source
	};
	// clang-format on
	PYPP_INIT(
	    PopulationPlacementPriority m_population_placement_priority,
	    PopulationPlacementPriority::target_and_source);

	bool operator==(ClusterByPopulationConnectivity const& rhs) const;

protected:
	/**
	 * @brief some NBs get removed, and a first sorting a conducted
	 **/
	void initialise() PYPP_OVERRIDE;

	/**
	 * @brief populations are sorted, neuron blocks are sorted, priority queue is handled,
	 **/
	void loop() PYPP_OVERRIDE;

	/**
	 * @brief the unplaced priority queue is put back to the placement queue
	 **/
	void finalise() PYPP_OVERRIDE;

	/**
	 * @brief handle the new placement, to be used later.
	 *        • stores the location of the PopulationSlice
	 *        • stores what populations were placed
	 *        • calls: calculation of the degree to related populations
	 *        • calls: update of the priority list in relation to the just placed population
	 * @param [in] NeuronPlacementRequest chunk: the PlacementRequest that was just placed.
	 * @param [in] halco::hicann::v2::NeuronBlockOnWafer nb: the NeuronBlock that was used for the
	 *placement.
	 *
	 **/
	void update_relations_to_placement(
	    NeuronPlacementRequest const& chunk,
	    halco::hicann::v2::NeuronBlockOnWafer const& nb) PYPP_OVERRIDE;

	/**
	 * @brief sorts the populations by the degree stored in m_precalculated_degree
	 **/
	virtual void sort_population_priority();

	/**
	 * @brief adds the first population to the m_prio list.
	 **/
	virtual void add_first_population_to_priority_list();

	/**
	 * @brief add new populations to the priority queue and updates the degrees of pops related to
	 *placed pops
	 *
	 * @param[in] NeuronPlacementRequest chunk: this NeuronPlacementRequest was placed, so we only
	 *need to search for relations to this one
	 **/
	virtual void update_population_priority_list(NeuronPlacementRequest const& chunk);

	/**
	 * @brief Sorts the Neuron Blocks, so it selects which neuron block shall be used next.
	 **/
	virtual void sort_neuron_blocks();

#ifndef PYPLUSPLUS
	virtual bool nb_order_function(
	    halco::hicann::v2::NeuronBlockOnWafer const& a,
	    halco::hicann::v2::NeuronBlockOnWafer const& b,
	    std::function<bool(
	        halco::hicann::v2::NeuronBlockOnWafer const&,
	        halco::hicann::v2::NeuronBlockOnWafer const&)> const& order) const;


	virtual bool hicann_order_function(
	    halco::hicann::v2::HICANNOnWafer const& a,
	    halco::hicann::v2::HICANNOnWafer const& b,
	    std::function<bool(
	        halco::hicann::v2::HICANNOnWafer const&, halco::hicann::v2::HICANNOnWafer const&)> const&
	        order) const;

	virtual bool neuron_blocks_comparator_function(
	    halco::hicann::v2::NeuronBlockOnWafer const& a,
	    halco::hicann::v2::NeuronBlockOnWafer const& b,
	    std::function<bool(
	        halco::hicann::v2::HICANNOnWafer const&, halco::hicann::v2::HICANNOnWafer const&)> const&
	        hicann_order,
	    std::function<bool(
	        halco::hicann::v2::NeuronBlockOnWafer const&,
	        halco::hicann::v2::NeuronBlockOnWafer const&)> const& nb_order) const;
#endif

	/**
	 * @brief calculates the center of the spiral.
	 *
	 * it takes communication partners into account and returns the arithmetric mean of their
	 *position. The return values are used to center other placements close to that location.
	 *
	 * @return pair<double,double> : containing x and y of the average position of partners
	 **/
	virtual std::pair<double, double> center_of_partners() const;


	/**
	 * @brief calculates the degree of a given Population.
	 * it uses the m_placed_mset multiset to have quick access to placed populations.
	 * @param [in] const graph_t::vertex_descriptor & pop: The population to calculate the degree
	 *for
	 * @return size_t: the degree
	 **/
#ifndef PYPLUSPLUS
	virtual size_t degree_to_placed(graph_t::vertex_descriptor const& pop) const;
#endif

	/**
	 * @brief overloaded version of @degree_to_placed. It converts a NeuronPlacmentRequest to the
	 *populations `vertex_descriptor`
	 * @param [in] const NeuronPlacementRequest & req: The population to calculate the degree for
	 * @return size_t: the degree
	 **/
	virtual size_t degree_to_placed(NeuronPlacementRequest const& req) const;

	std::vector<NeuronPlacementRequest> m_unconnected;

#ifndef PYPLUSPLUS
	mutable std::unordered_map<size_t /*Population->id()*/, size_t /*placement amount*/>
	    m_placed_mset;
	std::unordered_multimap<NeuronPlacementRequest, halco::hicann::v2::NeuronBlockOnWafer> m_placed;
	std::unordered_map<graph_t::vertex_descriptor, size_t> m_precalculated_degree;
#endif

}; // PlacePopulations


} // namespace internal
} // namespace placement
} // namespace marocco
