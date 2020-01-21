#pragma once

#include <array>
#include <iosfwd>
#include <memory>

#include <boost/iterator/iterator_adaptor.hpp>

#include "halco/hicann/v2/neuron.h"
#include "marocco/util/iterable.h"
#include "marocco/placement/internal/NeuronPlacementRequest.h"

namespace marocco {
namespace placement {
namespace internal {

namespace detail {
namespace on_neuron_block {

class iterator;
class neuron_iterator;

} // namespace on_neuron_block
} // namespace detail

/**
 * @brief Manages the assignment of population slices to the denmems of a neuron block.
 */
class OnNeuronBlock {
public:
	typedef halco::hicann::v2::NeuronOnNeuronBlock neuron_coordinate;
	typedef std::shared_ptr<NeuronPlacementRequest> value_type;
	typedef std::array<std::array<value_type, neuron_coordinate::y_type::size>,
	                   neuron_coordinate::x_type::size> array_type;
	typedef array_type::value_type::const_iterator base_iterator;
	typedef detail::on_neuron_block::iterator iterator;
	typedef detail::on_neuron_block::neuron_iterator neuron_iterator;

public:
	OnNeuronBlock();

	/**
	 * @brief Mark denmem as defect.
	 * @throw runtime_error If #add() has already been called.
	 * @throw ResourceInUseError If denmem has already been assigned to.
	 */
	void add_defect(neuron_coordinate const& nrn);

	/**
	 * @brief Place population slice on this neuron block.
	 * @return Iterator pointing at the assigned population slice if successful
	 *         or past-the-end iterator if not.
	 * @note
	 * - Assignments will always start at the top neuron row.
	 * - Only adjacent denmems will be used, in down-right order, i.e.:
	 *   \code
	 *   | 1 | 3 | 5   |
	 *   | 2 | 4 | ... |
	 *   \endcode
	 */
	iterator add(NeuronPlacementRequest const& value);

	/**
	 * @brief Place population slice on this neuron block, starting at the upper denmem of
	 *        the given column.
	 * @return Iterator pointing at the assigned population slice if successful
	 *         or past-the-end iterator if not.
	 * @see Only a continuous block of the correct size will be considered.
	 */
	iterator add(neuron_coordinate::x_type const& column, NeuronPlacementRequest const& value);

	/**
	 * @brief Return whether the given denmem is marked as defect.
	 */
	bool is_defect(neuron_coordinate const& nrn) const;

	/**
	 * @brief Return the population slice assigned to a denmem.
	 */
	value_type operator[](neuron_coordinate const& nrn) const;

	bool operator==(OnNeuronBlock const& other) const;

	/**
	 * @brief Return an iterator pointing at the population slice assigned to a denmem.
	 * @note Returns a past-the-end iterator if the denmem is defect
	 *       or does not have an assigned population.
	 */
	iterator get(neuron_coordinate const& nrn) const;

	/**
	 * @brief Return whether there are no assignments.
	 */
	bool empty() const;

	/**
	 * @brief Return the number of unassigned and non-defect denmems.
	 * @note #add() may nevertheless fail since there are constraints on
	 *       where assignments may start.
	 */
	size_t available() const;

	/**
	 * @brief Artificially restrict the number of available denmems.
	 * @note If #restrict() is called multiple times, the minimum value seen for
	        \c max_denmems will be kept.
	 * @return Value of restriction that is in place.
	 */
	size_t restrict(size_t max_denmems);

	/**
	 * @brief Iterate over assigned population slices.
	 * Every population slice will appear exactly once.
	 * @note Iterators can be passed in to #neurons() to iterate over the denmems assigned
	 *       to a given population slice.
	 */
	iterator begin() const;
	iterator end() const;

	/**
	 * @brief Iterate over the denmems of a population slice.
	 * @note Iteration starts at the top-left neuron and follows the
	 *       pattern of assignment described in #add():
	 *       \code
	 *       | 1 | 3 | 5   |
	 *       | 2 | 4 | ... |
	 *       \endcode
	 */
	iterable<neuron_iterator> neurons(iterator const& it) const;

private:
	static bool unassigned_p(value_type const& val)
	{
		return val == nullptr;
	}

	static bool assigned_p(value_type const& val)
	{
		return val != nullptr;
	}

	/**
	 * @brief Returns marker used to block defect denmems from being placed to.
	 * @note When checking a spot for a defect marker, pointer values are compared
	 *       so the exact value stored in the shared pointer is insignificant.
	 */
	static value_type defect_marker();

	/**
	 * @brief 2D array of NeuronPlacementRequests that were fulfilled by using the
	 *        respective denmems of the neuron block.
	 * @note In contrast to the general HALbe policy of row-first access, column-first
	 *       access is used here, as it captures the filling pattern outlined above.
	 */
	array_type mAssignment;

	/**
	 * @brief Number of assigned denmems.
	 * @note Denmems marked as defect do not count as assigned.
	 */
	size_t mSize;

	/**
	 * @brief Count of available (i.e. non-defect) denmems, if no populations
	 *        were assigned yet.
	 */
	size_t mAvailable;

	/**
	 * @brief Maximum count of denmems that should receive an assignment.
	 * @see #restrict()
	 */
	size_t mCeiling;
};

std::ostream& print(
	std::ostream& os,
	OnNeuronBlock const& onb,
	halco::hicann::v2::NeuronBlockOnHICANN const& nb);

namespace detail {
namespace on_neuron_block {

class iterator
    : public boost::iterator_adaptor<iterator, OnNeuronBlock::base_iterator,
                                     boost::use_default, boost::forward_traversal_tag> {
public:
	iterator(OnNeuronBlock::base_iterator const& it,
	         OnNeuronBlock::base_iterator const& end,
	         OnNeuronBlock::value_type const& defect);

private:
	friend class boost::iterator_core_access;
	void increment();
	OnNeuronBlock::base_iterator mEnd;
	OnNeuronBlock::value_type mDefect;
};

/**
 * @brief Iterates over the denmems of a population slice.
 * @note This assumes that the neurons of a population slice are assigned
 *       to consecutive denmems with no interspersed defects.
 * @note Dereferencing the iterator does not return a reference but a value!
 */
class neuron_iterator
    : public boost::iterator_adaptor<neuron_iterator,
                                     OnNeuronBlock::base_iterator,
                                     OnNeuronBlock::neuron_coordinate /* Value */,
                                     boost::forward_traversal_tag,
                                     // Dereferecing the iterator returns a value:
                                     OnNeuronBlock::neuron_coordinate /* Reference */> {
public:
	neuron_iterator(OnNeuronBlock::base_iterator const& it,
	                OnNeuronBlock::base_iterator const& beg,
	                OnNeuronBlock::base_iterator const& end);

private:
	friend class boost::iterator_core_access;
	void increment();
	OnNeuronBlock::neuron_coordinate dereference() const;
	OnNeuronBlock::base_iterator mBeg;
	OnNeuronBlock::base_iterator mEnd;
};

} // namespace on_neuron_block
} // namespace detail

} // namespace internal
} // namespace placement
} // namespace marocco
