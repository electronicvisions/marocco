#pragma once

#include <array>
#include <boost/iterator/iterator_adaptor.hpp>
#include <memory>

#include "hal/Coordinate/HMFGeometry.h"
#include "marocco/util.h"
#include "marocco/placement/NeuronPlacement.h"

namespace marocco {
namespace placement {

namespace detail {
namespace on_neuron_block {

class iterator;
class neuron_iterator;

} // namespace on_neuron_block
} // namespace detail

/// This class manages the assignment of populations to the hardware neurons
/// of a neuron block.
class OnNeuronBlock {
public:
	typedef HMF::Coordinate::NeuronOnNeuronBlock neuron_coordinate;
	typedef std::shared_ptr<NeuronPlacement> value_type;
	typedef std::array<std::array<value_type, neuron_coordinate::y_type::size>,
	                   neuron_coordinate::x_type::size> array_type;
	typedef array_type::value_type::const_iterator base_iterator;
	typedef detail::on_neuron_block::iterator iterator;
	typedef detail::on_neuron_block::neuron_iterator neuron_iterator;

public:
	OnNeuronBlock();

	/** Mark neuron as defect.
	 *  @throw ResourceInUseError If neuron has already been assigned.
	 */
	void add_defect(neuron_coordinate const& nrn);

	/** Place population on this neuron block.
	 *  @return Iterator pointing at the assigned population if successful
	 *          or past-the-end iterator if not.
	 *  @note
	 *  - Assignments will always start at the top neuron row.
	 *  - Only adjacent neurons will be used, in down-right order, i.e.:
	 *    \code
	 *    | 1 | 3 | 5   |
	 *    | 2 | 4 | ... |
	 *    \endcode
	 */
	iterator add(NeuronPlacement const& value);

	/** Return the population assigned to a hardware neuron.
	 */
	value_type operator[](neuron_coordinate const& nrn) const;

	/** Return an iterator pointing at the population assigned to a
	 *  hardware neuron.
	 *  @note Returns a past-the-end iterator if the neuron is defect
	 *        or does not have an assigned population.
	 */
	iterator get(neuron_coordinate const& nrn) const;

	/** Return true if there are neither defects nor assignments. */
	bool empty() const;

	/** Return the number of unassigned and non-defect hardware neurons.
	 *  @note #add() may nevertheless fail since there are constraints
	 *  on where assignments may start.
	 */
	size_t available() const;

	/** Iterate over assigned populations.
	 *  Every population will appear exactly once.
	 *  @note Iterators can be passed in to #neurons() to iterate over
	 *        the hardware neurons of a population.
	 */
	iterator begin() const;
	iterator end() const;

	/** Iterate over the hardware neurons of a population.
	 *  @note Iteration starts at the top-left neuron and follows the
	 *        pattern of assignment described in #add():
	 *        \code
	 *        | 1 | 3 | 5   |
	 *        | 2 | 4 | ... |
	 *        \endcode
	 */
	iterable<neuron_iterator> neurons(iterator const& it) const;

private:
	static bool unassigned_p(value_type const& val) { return !val; }
	static bool assigned_p(value_type const& val) { return !!val; }

	/** Member to mark defect hardware neurons:
	 * Implemented via a NeuronPlacement with hardware neuron size 2 from a 
	 * PopulationSlice of an empty Population.
	 *
	 * TODO: why not implemented as static member?
	 */
	value_type mDefect;

	/** 2D array of NeuronPlacements to the hardware neurons of the NeuronBlock.
	 * @note the 2D array is represented in column-first access way, in contrast
	 *       to HALBE policy of row-first access. Hence, neurons are accessed as
	 *       follows: mAssignment[xcoord][ycoord]
	 */
	array_type mAssignment;

    /** count of available hardware neurons, i.e. neurons that are neither marked as defect, nor assigned already.
	 */
	size_t mAvailable;
};

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

/** Iterates over the hardware neurons of a population.
 *  @note This assumes that the neurons of a population are assigned
 *        to consecutive hardware neurons with no interspersed defects.
 *  @note Dereferencing the iterator does not return a value but a reference!
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

} // namespace placement
} // namespace marocco
