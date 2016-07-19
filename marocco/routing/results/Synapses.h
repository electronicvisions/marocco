#pragma once

#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/optional.hpp>
#include <boost/serialization/export.hpp>

#include "hal/Coordinate/Synapse.h"

#include "marocco/coordinates/BioNeuron.h"
#include "marocco/util/iterable.h"

namespace boost {
namespace serialization {
class access;
} // namespace serialization
} // namespace boost

namespace marocco {
namespace routing {
namespace results {

class Synapses {
public:
	typedef size_t projection_type;
	typedef HMF::Coordinate::SynapseOnWafer hardware_synapse_type;
	typedef boost::optional<hardware_synapse_type> optional_hardware_synapse_type;

	/**
	 * @brief Representation of a single placed synapse.
	 * As pynn supports unique identifiers for single neurons but has no way to address
	 * single connections in a projection, the biological equivalent to a hardware synapse
	 * is encoded via its source and target neurons.  However, marocco allows for multapses
	 * via parallel edges in its graph representation of the biological network, thus we
	 * need to keep the projection identifier, too.
	 * The only identifiers acccessible to the user are the euter IDs of the user-created
	 * assemblies/projections.  Marocco “flattens” the user-created projection and uses
	 * the obtained projection views as edges in its graph representation.  Those contain
	 * a reference to their “parent” projection, but do not directly translate to objects
	 * known to the user.
	 * Note that eventually multiple hardware synapses may belong to a single connection
	 * of a projection to increase the dynamic range of synapse weights.
	 */
	class item_type {
	public:
		/**
		 * @param projection Euter ID of biological projection.
		 */
		item_type(
			projection_type projection,
			BioNeuron const& source_neuron,
			BioNeuron const& target_neuron,
			hardware_synapse_type const& hardware_synapse);

		item_type(
			projection_type projection,
			BioNeuron const& source_neuron,
			BioNeuron const& target_neuron);

		projection_type projection() const;
		BioNeuron const& source_neuron() const;
		BioNeuron const& target_neuron() const;
		optional_hardware_synapse_type const& hardware_synapse() const;

	private:
		projection_type m_projection;
		BioNeuron m_source_neuron;
		BioNeuron m_target_neuron;
		optional_hardware_synapse_type m_hardware_synapse;

		friend class boost::serialization::access;
		item_type();
		template <typename Archiver>
		void serialize(Archiver& ar, const unsigned int /* version */);
	}; // item_type

	typedef boost::multi_index::multi_index_container<
	    item_type,
	    boost::multi_index::indexed_by<
	        // We use ordered indices to implement lookup by synapse row.
	        boost::multi_index::ordered_non_unique<
	            boost::multi_index::tag<hardware_synapse_type>,
	            boost::multi_index::const_mem_fun<item_type,
	                                              optional_hardware_synapse_type const&,
	                                              &item_type::hardware_synapse> >,
	        boost::multi_index::hashed_non_unique<
	            boost::multi_index::tag<projection_type>,
	            boost::multi_index::composite_key<
	                item_type,
	                boost::multi_index::
	                    const_mem_fun<item_type, projection_type, &item_type::projection>,
	                boost::multi_index::
	                    const_mem_fun<item_type, BioNeuron const&, &item_type::source_neuron>,
	                boost::multi_index::
	                    const_mem_fun<item_type, BioNeuron const&, &item_type::target_neuron> > >,
	        boost::multi_index::hashed_non_unique<
	            boost::multi_index::tag<BioNeuron>,
	            boost::multi_index::composite_key<
	                item_type,
	                boost::multi_index::
	                    const_mem_fun<item_type, BioNeuron const&, &item_type::source_neuron>,
	                boost::multi_index::const_mem_fun<item_type,
	                                                  BioNeuron const&,
	                                                  &item_type::target_neuron> > > > >
	    container_type;
	typedef container_type::index<BioNeuron>::type by_neurons_type;
	typedef container_type::index<projection_type>::type by_projection_and_neurons_type;
	typedef container_type::index<hardware_synapse_type>::type by_hardware_synapse_type;
	typedef container_type::iterator iterator;
	typedef container_type::iterator const_iterator;

	void add(
		projection_type projection,
		BioNeuron const& source_neuron,
		BioNeuron const& target_neuron,
		hardware_synapse_type const& hardware_synapse);

	void add_unrealized_synapse(
		projection_type projection,
		BioNeuron const& source_neuron,
		BioNeuron const& target_neuron);

	/**
	 * @brief Find all synapses connecting the given bio neurons.
	 * @note This may return multiple synapses, see \ref item_type.
	 */
	iterable<by_neurons_type::iterator> find(
	    BioNeuron const& source_neuron, BioNeuron const& target_neuron) const;

	/**
	 * @brief Find all synapses connecting the given bio neurons.
	 * @note This may return multiple synapses, see \ref item_type.
	 */
	iterable<by_projection_and_neurons_type::iterator> find(
		projection_type projection,
		BioNeuron const& source_neuron,
		BioNeuron const& target_neuron) const;

	iterable<by_hardware_synapse_type::iterator> find(
		HMF::Coordinate::SynapseOnWafer const& hardware_synapse) const;

	iterable<by_hardware_synapse_type::iterator> find(
		HMF::Coordinate::HICANNOnWafer const& hicann,
		HMF::Coordinate::SynapseRowOnHICANN const& synapse_row) const;

	iterable<by_hardware_synapse_type::iterator> unrealized_synapses() const;

	bool empty() const;

	size_t size() const;

	iterator begin() const;

	iterator end() const;

private:
	container_type m_container;

	friend class boost::serialization::access;
	template <typename Archiver>
	void serialize(Archiver& ar, const unsigned int /* version */);
}; // Synapses

PYPP_INSTANTIATE(iterable<Synapses::by_neurons_type::iterator>)
PYPP_INSTANTIATE(iterable<Synapses::by_projection_and_neurons_type::iterator>)
PYPP_INSTANTIATE(iterable<Synapses::by_hardware_synapse_type::iterator>)

} // namespace results
} // namespace routing
} // namespace marocco

BOOST_CLASS_EXPORT_KEY(::marocco::routing::results::Synapses)
BOOST_CLASS_EXPORT_KEY(::marocco::routing::results::Synapses::item_type)
