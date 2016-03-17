#pragma once

#include <unordered_map>
#include <vector>
#include <boost/optional.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/mem_fun.hpp>

#include "hal/Coordinate/HICANN.h"
#include "hal/Coordinate/Neuron.h"
#include "hal/Coordinate/typed_array.h"
#include "marocco/coordinates/BioNeuron.h"
#include "marocco/coordinates/LogicalNeuron.h"
#include "marocco/graph.h"
#include "marocco/placement/OnNeuronBlock.h"
#include "marocco/util/iterable.h"

namespace marocco {
namespace placement {

/**
 * @brief Provides an implicit mapping of populations to denmems of a HICANN by combining
 *        the OnNeuronBlock mappings of all neuron blocks.
 */
typedef HMF::Coordinate::typed_array<OnNeuronBlock, HMF::Coordinate::NeuronBlockOnHICANN>
    NeuronBlockMapping;

class NeuronPlacementResult
{
public:
	typedef graph_t::vertex_descriptor vertex_descriptor;
	typedef std::unordered_map<HMF::Coordinate::HICANNOnWafer, NeuronBlockMapping>
		denmem_assignment_type;
	typedef std::unordered_map<vertex_descriptor, std::vector<HMF::Coordinate::NeuronOnWafer> >
		primary_denmems_for_population_type;

	class item_type {
	public:
		typedef boost::optional<HMF::Coordinate::NeuronBlockOnWafer> neuron_block_type;

		item_type(BioNeuron const& bio_neuron, LogicalNeuron const& logical_neuron);

		neuron_block_type const& neuron_block() const;
		BioNeuron const& bio_neuron() const;
		vertex_descriptor population() const;
		size_t neuron_index() const;
		LogicalNeuron const& logical_neuron() const;

	private:
		BioNeuron const m_bio_neuron;
		LogicalNeuron const m_logical_neuron;
		neuron_block_type const m_neuron_block;
	}; // item_type

	typedef boost::multi_index::multi_index_container<
	    item_type,
	    boost::multi_index::indexed_by<
	        boost::multi_index::hashed_unique<
	            boost::multi_index::tag<BioNeuron>,
	            boost::multi_index::
	                const_mem_fun<item_type, BioNeuron const&, &item_type::bio_neuron> >,
	        boost::multi_index::hashed_unique<
	            boost::multi_index::tag<LogicalNeuron>,
	            boost::multi_index::
	                const_mem_fun<item_type, LogicalNeuron const&, &item_type::logical_neuron> >,
	        boost::multi_index::hashed_non_unique<
	            boost::multi_index::tag<vertex_descriptor>,
	            boost::multi_index::
	                const_mem_fun<item_type, vertex_descriptor, &item_type::population> >,
	        boost::multi_index::hashed_non_unique<
	            boost::multi_index::tag<HMF::Coordinate::NeuronBlockOnWafer>,
	            boost::multi_index::const_mem_fun<
	                item_type,
		            item_type::neuron_block_type const&,
	                &item_type::neuron_block> > > >
	    container_type;
	typedef container_type::index<BioNeuron>::type by_bio_neuron_type;
	typedef container_type::index<LogicalNeuron>::type by_logical_neuron_type;
	typedef container_type::index<vertex_descriptor>::type by_population_type;
	typedef container_type::index<HMF::Coordinate::NeuronBlockOnWafer>::type by_neuron_block_type;

	void add(BioNeuron const& bio_neuron, LogicalNeuron const& logical_neuron);

	iterable<by_bio_neuron_type::iterator> find(BioNeuron const& bio_neuron) const;

	iterable<by_logical_neuron_type::iterator> find(LogicalNeuron const& logical_neuron) const;

	iterable<by_population_type::iterator> find(vertex_descriptor const& population) const;

	iterable<by_neuron_block_type::iterator> find(
	    boost::optional<HMF::Coordinate::NeuronBlockOnWafer> const& neuron_block) const;

	// This overload has been added because otherwise find(vertex_descriptor const&) would
	// be a viable candidate for a find(neuron_block) call, because of implicit conversion
	// to integral types.
	iterable<by_neuron_block_type::iterator> find(
	    HMF::Coordinate::NeuronBlockOnWafer const& neuron_block) const;

	by_population_type::iterator begin() const;

	by_population_type::iterator end() const;

	/**
	 * @brief Internal data structure used by neuron mapping.
	 * @deprecated Use #find() instead.
	 */
	denmem_assignment_type const& denmem_assignment() const;
	denmem_assignment_type& denmem_assignment();

	/**
	 * @brief Maps population vertices onto primary denmems.
	 * Those can be used to look up all connected denmems via OnNeuronBlock.
	 * @deprecated Use #find() instead.
	 */
	primary_denmems_for_population_type const& primary_denmems_for_population() const;
	primary_denmems_for_population_type& primary_denmems_for_population();

private:
	denmem_assignment_type m_denmem_assignment;
	primary_denmems_for_population_type m_primaries;
	container_type m_container;
}; // NeuronPlacementResult

} // namespace placement
} // namespace marocco
