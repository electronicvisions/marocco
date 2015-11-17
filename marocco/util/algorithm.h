#pragma once

#include <algorithm>
#include <iterator>

namespace marocco {
namespace algorithm {

/// Returns the number of consecutive elements in the range [first, last)
/// for which predicate p returns true.
template <typename InputIt, typename UnaryPredicate>
typename std::iterator_traits<InputIt>::difference_type
count_while(InputIt first, InputIt last, UnaryPredicate p)
{
	return std::find_if_not(first, last, p) - first;
}

template <typename InputIt>
double arithmetic_mean(InputIt begin, InputIt end)
{
	return std::accumulate(begin, end, 0.0) / std::distance(begin, end);
}

} // namespace algorithm
} // namespace marocco
