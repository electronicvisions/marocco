#pragma once

#include <array>
#include <stdexcept>
#include <boost/dynamic_bitset.hpp>

#include "marocco/config.h"
#include "marocco/util.h"
#include "hal/Coordinate/HMFGeometry.h"

namespace marocco {
namespace routing {

inline
size_t getPopulationViewOffset(size_t pop_offset, boost::dynamic_bitset<> const& mask)
{
	if (pop_offset>=mask.size()) {
		throw std::out_of_range("mask to short");
	}
	size_t cnt=0;
	for (size_t ii=0; ii<pop_offset; ++ii)
	{
		cnt += mask[ii];
	}
	return cnt;
}

inline
size_t getPopulationOffset(size_t pop_view_offset, boost::dynamic_bitset<> const& mask)
{
	if (pop_view_offset >= mask.count()) {
		throw std::out_of_range("mask to short");
	}
	size_t ii = 0;
	size_t cnt = 0;
	for (; ii < mask.size(); ++ii){
		cnt += mask[ii];
		if (cnt ==  pop_view_offset + 1)
			break;
	}
	return ii;
}

} // namespace routing
} // namespace marocco
