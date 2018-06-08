#include "marocco/routing/SynapseRowIterator.h"

using namespace HMF::Coordinate;

namespace marocco {
namespace routing {

SynapseRowIterator::SynapseRowIterator(assignment_iterator it, assignment_iterator eit) :
	ait(it), lait(eit), row(0)
{
	if (it!=eit) {
		dit = it->drivers.begin();
	}
}

typename SynapseRowIterator::facade::reference SynapseRowIterator::dereference() const
{
	SynapseDriverOnHICANN const& drv = *dit;
	return SynapseRowOnHICANN(drv, row);
}

void SynapseRowIterator::increment()
{
	// easy
	if (row==0) {
		row = row_index(1);
		return;
	}

	++dit;
	row = row_index(0);
	if (dit==ait->drivers.end())
	{
		++ait;
		if (ait<lait) {
			dit=ait->drivers.begin();
		}
	}
}

bool SynapseRowIterator::equal(SynapseRowIterator const rhs) const
{
	return  ait == rhs.ait
		&& lait == rhs.lait
		// dit is not well defined for end iterator ....
		&&  (ait==lait ? true : dit == rhs.dit)
		&&  row == rhs.row;
}

} // namespace routing
} // namespace marocco
