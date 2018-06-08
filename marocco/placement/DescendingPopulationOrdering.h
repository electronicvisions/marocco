#pragma once

#include "marocco/assignment/PopulationSlice.h"

namespace marocco {
namespace placement {

struct DescendingPopulationOrdering
{
	typedef assignment::PopulationSlice type;
	bool operator() (type const& a, type const& b) const;
};


} // namespace placement
} // namespace marocco
