#pragma once

#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/optional.hpp>
#include <boost/serialization/export.hpp>

#include "hal/Coordinate/HICANN.h"
#include "hal/Coordinate/Neuron.h"
#include "pywrap/compat/macros.hpp"
#include "marocco/coordinates/BioNeuron.h"
#include "marocco/coordinates/L1AddressOnWafer.h"
#include "marocco/coordinates/LogicalNeuron.h"
#include "marocco/util/iterable.h"

namespace boost {
namespace serialization {
class access;
} // namespace serialization
} // namespace boost

namespace marocco {
namespace placement {
namespace results {

class Placement {
public:
	typedef size_t vertex_descriptor;

	class item_type {
	public:
		typedef boost::optional<HMF::Coordinate::NeuronBlockOnWafer> neuron_block_type;
		typedef boost::optional<HMF::Coordinate::DNCMergerOnWafer> dnc_merger_type;
		typedef boost::optional<L1AddressOnWafer> address_type;

		item_type(BioNeuron const& bio_neuron, LogicalNeuron const& logical_neuron);

		neuron_block_type const& neuron_block() const;
		dnc_merger_type dnc_merger() const;
		BioNeuron const& bio_neuron() const;
		vertex_descriptor population() const;
		size_t neuron_index() const;
		LogicalNeuron const& logical_neuron() const;

		address_type const& address() const;
		void set_address(L1AddressOnWafer const& address);

	private:
		BioNeuron m_bio_neuron;
		LogicalNeuron m_logical_neuron;
		neuron_block_type m_neuron_block;
		address_type m_address;

		friend class boost::serialization::access;
		item_type();
		template <typename Archiver>
		void serialize(Archiver& ar, const unsigned int /* version */);
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
	        boost::multi_index::ordered_non_unique<
	            boost::multi_index::tag<HMF::Coordinate::NeuronBlockOnWafer>,
	            boost::multi_index::const_mem_fun<
	                item_type,
		            item_type::neuron_block_type const&,
	                &item_type::neuron_block> >,
	        boost::multi_index::hashed_non_unique<
	            boost::multi_index::tag<HMF::Coordinate::DNCMergerOnWafer>,
	            boost::multi_index::const_mem_fun<
	                item_type,
		            item_type::dnc_merger_type,
	                &item_type::dnc_merger> > > >
	    container_type;
	typedef container_type::index<BioNeuron>::type by_bio_neuron_type;
	typedef container_type::index<LogicalNeuron>::type by_logical_neuron_type;
	typedef container_type::index<vertex_descriptor>::type by_population_type;
	typedef container_type::index<HMF::Coordinate::NeuronBlockOnWafer>::type by_neuron_block_type;
	typedef container_type::index<HMF::Coordinate::DNCMergerOnWafer>::type by_dnc_merger_type;
	typedef container_type::iterator iterator;
	typedef container_type::iterator const_iterator;

	void add(BioNeuron const& bio_neuron, LogicalNeuron const& logical_neuron);

	iterable<by_bio_neuron_type::iterator> find(BioNeuron const& bio_neuron) const;

	iterable<by_logical_neuron_type::iterator> find(LogicalNeuron const& logical_neuron) const;

	iterable<by_population_type::iterator> find(vertex_descriptor const& population) const;

	iterable<by_neuron_block_type::iterator> find(
	    boost::optional<HMF::Coordinate::NeuronBlockOnWafer> const& neuron_block) const;

	iterable<by_dnc_merger_type::iterator> find(
	    boost::optional<HMF::Coordinate::DNCMergerOnWafer> const& dnc_merger) const;

	// The overloads without boost::optional<T> have been added because otherwise
	// find(vertex_descriptor const&) would be a viable candidate for a find(T{}) call,
	// because of implicit conversion to integral types.

	iterable<by_neuron_block_type::iterator> find(
	    HMF::Coordinate::NeuronBlockOnWafer const& neuron_block) const;

	iterable<by_dnc_merger_type::iterator> find(
	    HMF::Coordinate::DNCMergerOnWafer const& dnc_merger) const;

	/**
	 * @brief Find all placements with neuron blocks on the given HICANN.
	 */
	iterable<by_neuron_block_type::iterator> find(
		HMF::Coordinate::HICANNOnWafer const& hicann) const;

	bool empty() const;

	size_t size() const;

	iterator begin() const;

	iterator end() const;

	/**
	 * @brief Set L1 address of specified logical neuron.
	 * @throw std::out_of_range If no corresponding placement could be found.
	 * @throw std::runtime_error If there are conflicting addresses.
	 */
	void set_address(LogicalNeuron const& logical_neuron, L1AddressOnWafer const& address);

private:
	container_type m_container;

	friend class boost::serialization::access;
	template <typename Archiver>
	void serialize(Archiver& ar, const unsigned int /* version */);
}; // Placement

PYPP_INSTANTIATE(iterable<Placement::by_bio_neuron_type::iterator>)
PYPP_INSTANTIATE(iterable<Placement::by_logical_neuron_type::iterator>)
PYPP_INSTANTIATE(iterable<Placement::by_population_type::iterator>)
PYPP_INSTANTIATE(iterable<Placement::by_neuron_block_type::iterator>)
PYPP_INSTANTIATE(iterable<Placement::by_dnc_merger_type::iterator>)

} // namespace results
} // namespace placement
} // namespace marocco

BOOST_CLASS_EXPORT_KEY(::marocco::placement::results::Placement)
BOOST_CLASS_EXPORT_KEY(::marocco::placement::results::Placement::item_type)
