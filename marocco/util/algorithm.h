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

template <class InputIt, class T>
typename std::iterator_traits<InputIt>::difference_type
index_of(InputIt first, InputIt last, const T& value)
{
	return std::distance(first, std::find(first, last, value));
}

template <typename InputIt>
double arithmetic_mean(InputIt begin, InputIt end)
{
	return std::accumulate(begin, end, 0.0) / std::distance(begin, end);
}

} // namespace algorithm
} // namespace marocco
