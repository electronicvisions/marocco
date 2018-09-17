#pragma once

#include <vector>

#include <boost/optional.hpp>
#include <boost/serialization/export.hpp>

#ifndef PYPLUSPLUS
#include "marocco/graph.h" // includes boost/graph/adjecencylist.hpp not parsable by gccxml
#include "marocco/placement/internal/NeuronPlacementRequest.h"
#include "marocco/placement/internal/Result.h"
#endif // !PYPLUSPLUS
#include "hal/Coordinate/Neuron.h"


namespace marocco {
namespace placement {
namespace internal {
class NeuronPlacementRequest;
class OnNeuronBlock;
}
}
}

namespace HMF {
namespace Coordinate {
class NeuronOnWafer;
}
}

using marocco::placement::internal::NeuronPlacementRequest;
using marocco::placement::internal::OnNeuronBlock;

namespace boost {
namespace serialization {
class access;
}
} // namespace boost::serialization

namespace marocco {
namespace placement {
namespace algorithms {

/** Placement of populations to a list of neuron blocks.
 *
 * This is a Base class from which specific placement strategies can be derived.
 * the base handles the general workflow.
 * It places Population Slices on NeuronBlocks. Derived classes shall handle
 * the sorting of these two queues.
 * To do so, virtual functions are provided for the derived classes to use.
 *
 */
class PlacePopulationsBase
{
public:
	typedef HMF::Coordinate::NeuronOnWafer result_type;

	/**
	 * @brief This function should run until all populations of the queue are placed
	 *
	 * 		Derived classes might want to change it, to change Neuronblock or Population ordering
	 * 		as placement continues, or use the _init()_ and _loop()_ hooks.
	 *
	 * @param graph
	 *        BioGraph representation of the network.
	 * @param state
	 *        Container with hardware configuration.
	 *        Will be modified as placement progresses.
	 * @param neuron_blocks
	 *        Neuron blocks to use for placement.
	 *        Used-up blocks will be removed.
	 * @param queue
	 *        Population slices that are to be placed.
	 *        If placement fails, some requests may remain in this queue.
	 *        It is important, that not placed PlacementRequests remain in the queue.
	 */
#ifndef PYPLUSPLUS
	boost::optional<std::vector<PlacePopulationsBase::result_type> > run(
	    graph_t const& graph,
	    internal::Result::denmem_assignment_type& state,
	    std::vector<HMF::Coordinate::NeuronBlockOnWafer>& neuron_blocks,
	    std::vector<NeuronPlacementRequest>& queue);
#endif // !PYPLUSPLUS
protected:
	/**
	 * @brief a hook for derived classes.
	 * 		this function is executed once after the state of base class has been set,
	 * 		and before the first place_one_population
	 **/
	virtual void initialise(){};

	/**
	 * @brief a hook for derived classes.
	 * 		this function is executed after every place_one_population();
	 **/
	virtual void loop(){};

	/**
	 * @brief a hook for derived classes.
	 * 		this function is executed right before the result is returned.
	 * 		this means it is executed after all populations have been placed (or not).
	 **/
	virtual void finalise(){};

	/**
	 * @brief this hook is used to notify derived classes of a successful placement.
	 *
	 * this is especially useful in the Cluster placement, as it requires informations on the
	 *placement of populations.
	 **/
	virtual void update_relations_to_placement(
	    NeuronPlacementRequest const& chunk, HMF::Coordinate::NeuronBlockOnWafer const& nb)
	{
		static_cast<void>(chunk);
		static_cast<void>(nb);
	};


	/**
	 * @brief selects the last Population and places it on the last Neuron Block.
	 *		Derived Classes most likely want to hook into init() and loop()
	 * 		to influence the order of populations and neuron blocks.
	 **/
	bool place_one_population();


#ifndef PYPLUSPLUS
	// the bio graph of the network, derived classes use it for cluster analysis
	boost::optional<graph_t const&> m_bio_graph;
	// stores all primary neurons, used later in post processing
	boost::optional<std::vector<result_type> > m_result;
	// used to get references to OnNeuronBlocks
	boost::optional<internal::Result::denmem_assignment_type&> m_state;
#endif // !PYPLUSPLUS

public: // compilation error W1039: `Py++` doesn't expose private or protected member variables.
	// the neuron blocks that may be used. might be all or restricted due to manual placement
	// request.
	boost::optional<std::vector<HMF::Coordinate::NeuronBlockOnWafer>&> m_neuron_blocks;
	// the population slices that shall be placed.
	boost::optional<std::vector<NeuronPlacementRequest>&> m_queue;

private:
	friend class boost::serialization::access;
	template <typename Archive>
	void serialize(Archive& ar, unsigned int const /* version */);

}; // PlacePopulationsBase

} // namespace internal
} // namespace placement
} // namespace marocco

BOOST_CLASS_EXPORT_KEY(::marocco::placement::algorithms::PlacePopulationsBase)
