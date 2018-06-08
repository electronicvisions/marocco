#pragma once

#include <boost/iterator/iterator_facade.hpp>

#include "marocco/routing/SynapseDriverSA.h"

namespace marocco {
namespace routing {

/// This iterator can be used to iterate over a Result with assigned drivers to
/// retrieve the corresponding SynapseRows in lexicographical order. Note, that
/// this is not really an interator in the sense, that it returns a reference to
/// some container content, instead it returns coordinates.
class SynapseRowIterator :
	public boost::iterator_facade<
		SynapseRowIterator,
		HMF::Coordinate::SynapseRowOnHICANN const,
		boost::forward_traversal_tag,
		HMF::Coordinate::SynapseRowOnHICANN>
{
public:
	typedef boost::iterator_facade<
			SynapseRowIterator,
			HMF::Coordinate::SynapseRowOnHICANN const,
			boost::forward_traversal_tag,
			HMF::Coordinate::SynapseRowOnHICANN /* deref to value */
		> facade;

	typedef DriverConfigurationState::Result::mapped_type::const_iterator
		assignment_iterator;
	typedef DriverAssignment::Drivers::const_iterator driver_iterator;
	typedef HMF::Coordinate::RowOnSynapseDriver row_index;

	/// init new iterator with a result
	SynapseRowIterator(assignment_iterator it,
					   assignment_iterator eit);

	/// default default & copy constructor
	SynapseRowIterator() = default;
	SynapseRowIterator(SynapseRowIterator const&) = default;


	typename facade::reference dereference() const;
	void increment();
	bool equal(SynapseRowIterator const rhs) const;

private:
	/// indexes the DriverAssignment.
	assignment_iterator ait;
	assignment_iterator const lait;

	/// indexes the a Driver within the Assignment.
	driver_iterator dit;

	/// indexes one of the two corresponding rows of that driver;
	row_index row;
};

} // namespace routing
} // namespace marocco
