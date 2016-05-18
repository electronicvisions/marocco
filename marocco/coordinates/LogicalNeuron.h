#pragma once

#include <iostream>
#include <set>
#include <utility>
#include <boost/function.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/operators.hpp>
#include <boost/serialization/export.hpp>

#include "hal/Coordinate/Neuron.h"
#include "hal/Coordinate/Wafer.h"
#include "marocco/util/iterable.h"

namespace marocco {

class LogicalNeuron;

namespace detail {

template <typename T>
struct pretty_printer;
std::ostream& operator<<(std::ostream& os, pretty_printer<LogicalNeuron> pr);

} // namespace detail

/**
 * @brief Hardware neuron corresponding to a biological neuron.
 * A LogicalNeuron represents either a collection of connected denmems implementing one
 * neuron of a biological population or an external input / neuron encoded by a unique
 * identifier.
 * @invariant Denmem chunks are sorted by their offset, non-overlapping, connected and do
 *            not exceed neuron block boundaries.
 */
class LogicalNeuron : boost::equality_comparable<LogicalNeuron>
{
	template <typename T>
	struct compare_on_first
	{
		bool operator()(T const& lhs, T const& rhs) const
		{
			return lhs.first < rhs.first;
		}
	};

public:
	typedef HMF::Coordinate::NeuronOnWafer neuron_type;
	typedef HMF::Coordinate::NeuronBlockOnWafer neuron_block_type;
	typedef HMF::Coordinate::NeuronOnNeuronBlock neuron_on_block_type;
	/**
	 * @brief Chunk of neurons starting at offset with specified width/size.
	 * @note Chunks can not span multiple rows of neurons, thus the maximum size of a
	 *       chunk is constrained by the number of neurons per row of a neuron block.
	 */
	typedef std::pair<neuron_on_block_type, size_t> chunk_type;
	/**
	 * @brief Identifier of external neuron.
	 * Positive values are reserved for spike input populations represented by marocco,
	 * negative values are available for external use.
	 */
	typedef int external_identifier_type;

private:
	typedef std::set<chunk_type, compare_on_first<chunk_type> > container_type;

public:
	class iterator : public boost::iterator_facade<iterator,
	                                               neuron_type,
	                                               boost::forward_traversal_tag,
	                                               // Return copy instead of reference:
	                                               neuron_type>
	{
		typedef size_t offset_type;
		typedef container_type::const_iterator underlying_iterator;

	public:
		iterator(neuron_block_type const& block, underlying_iterator const& chunk_iterator);

	private:
		friend class boost::iterator_core_access;

		bool equal(iterator const& other) const;
		void increment();
		neuron_type dereference() const;

		neuron_block_type m_block;
		underlying_iterator m_chunk_iterator;
		offset_type m_offset;
	};

	/**
	 * @brief Factory class to create a LogicalNeuron.
	 * As the comparison function used in the container intentionally only checks the
	 * offset (and not the size) of a chunk, we have to ensure that only one chunk starts
	 * at a certain offset to fulfill the invariants of LogicalNeuron.
	 * Thus we encapsulate checking of the return value of std::set::insert() in this builder.
	 */
	class Builder
	{
	public:
		/**
		 * @see LogicalNeuron::on().
		 */
		explicit Builder(neuron_block_type const& block);

		/**
		 * @brief Adds a chunk of denmems to the to-be-created logical neuron.
		 * @throw std::invalid_argument
		 */
		Builder& add(neuron_on_block_type const& offset, size_t const size);

		/**
		 * @brief Combines subsequent chunks of the same row if they are touching.
		 */
		Builder& squash();

		/**
		 * @brief Finishes adding chunks and construct a LogicalNeuron.
		 * @throw std::invalid_argument
		 * @post Builder object is in same state as directly after construction,
		 *       i.e. it can be reused.
		 */
		LogicalNeuron done();

		/**
		 * @brief Adds a chunk of denmems to the to-be-created logical neuron.
		 * This function was added for compatibility with std::back_inserter().
		 * @throw std::invalid_argument
		 */
		void push_back(chunk_type const& chunk);
		typedef chunk_type value_type;

	private:
		neuron_block_type m_block;
		container_type m_chunks;
	};

	LogicalNeuron();

	/**
	 * @brief Constructs a logical neuron that represents a collection of denmems.
	 */
	static Builder on(neuron_block_type const& block);

	/**
	 * @brief Constructs a logical neuron that represents an external input.
	 * @param external_identifier Identifier of external neuron.
	 * Positive values are reserved for spike input populations represented by marocco,
	 * negative values are available for external use.
	 * @param index Optional index into external neuron.
	 */
	static LogicalNeuron external(
	    external_identifier_type const external_identifier, size_t const index = 0);

	/**
	 * @brief Returns whether this logical neuron represents an external input.
	 */
	bool is_external() const;

	/**
	 * @brief Returns the identifier of the external input.
	 * @throw std::runtime_error If this logical neuron represents a collection of denmems.
	 */
	external_identifier_type external_identifier() const;

	/**
	 * @brief Returns an optional index into the external neuron.
	 * @throw std::runtime_error If this logical neuron represents a collection of denmems.
	 */
	size_t external_index() const;

	/**
	 * @brief Returns the number of contained denmems.
	 */
	size_t size() const;

	/**
	 * @brief Returns the specified denmem.
	 * @note Note that denmems are sorted left-to-right top-to-bottom.
	 * @throw std::runtime_error If this logical neuron represents an external input.
	 * @throw std::out_of_range If the index is out of bounds.
	 */
	neuron_type denmem(size_t const index) const;

	/**
	 * @brief Returns an iterator to the beginning.
	 * @note Note that this in equivalent to #end() for logical neurons representing an
	 *       external input.
	 */
	iterator begin() const;

	/**
	 * @brief Returns an iterator to the beginning.
	 */
	iterator end() const;

	/**
	 * @brief Returns the first denmem.
	 * @throw std::runtime_error If this logical neuron represents an external input.
	 */
	neuron_type front() const;

	/**
	 * @brief Returns the last denmem.
	 * @throw std::runtime_error If this logical neuron represents an external input.
	 */
	neuron_type back() const;

	/**
	 * @brief Checks whether a non-external logical neuron is composed of a rectangular
	 *        arrangement of denmems.
	 * @throw std::runtime_error If this logical neuron represents an external input.
	 * In particular, this function checks whether this neuron has a rectangular single
	 * connected component.
	 */
	bool is_rectangular() const;

	size_t hash() const;

private:
	LogicalNeuron(external_identifier_type const external_identifier, size_t const index = 0);
#ifndef PYPLUSPLUS
	LogicalNeuron(neuron_block_type const& block, container_type&& chunks);
#endif // !PYPLUSPLUS

	friend std::ostream&
	detail::operator<<(std::ostream& os, detail::pretty_printer<LogicalNeuron> pr);
	friend bool operator==(LogicalNeuron const& lhs, LogicalNeuron const& rhs);

	/// Optional identifier of external neuron.
	size_t m_external_identifier;
	union {
		/// Overall number of denmems.
		size_t m_size;
		/// Index into external neuron.
		size_t m_external_index;
	};
	neuron_block_type m_block;
	container_type m_chunks;

	friend class boost::serialization::access;
	template <typename Archiver>
	void serialize(Archiver& ar, const unsigned int /* version */);
}; // LogicalNeuron

std::ostream& operator<<(std::ostream& os, LogicalNeuron const& nrn);
bool operator==(LogicalNeuron const& lhs, LogicalNeuron const& rhs);
size_t hash_value(LogicalNeuron const& nrn);

} // namespace marocco

BOOST_CLASS_EXPORT_KEY(::marocco::LogicalNeuron)

namespace std {

template <>
struct hash<marocco::LogicalNeuron>
{
	size_t operator()(marocco::LogicalNeuron const& nrn) const
	{
		return hash_value(nrn);
	}
};

} // namespace std
