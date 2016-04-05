#pragma once

#include <algorithm>
#include <cmath>

#include "marocco/util/algorithm.h"

namespace marocco {

template <typename T, typename OrderT>
class wafer_ordering {
public:
	wafer_ordering() : m_order()
	{
	}

	wafer_ordering(OrderT const& order) : m_order(order)
	{
	}

	bool operator()(T const& lhs, T const& rhs) const {
		if (lhs.toWafer() != rhs.toWafer()) {
			return lhs.toWafer() < rhs.toWafer();
		}
		return m_order(lhs, rhs);
	}

private:
	OrderT m_order;
}; // wafer_ordering

} // namespace marocco
