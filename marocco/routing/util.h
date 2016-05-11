#pragma once

#include <stdexcept>

namespace marocco {
namespace routing {

/**
 * @brief Convert index to index of subset.
 * @tparam T bitset, e.g. \c boost::dynamic_bitset<>.
 * @param mask Mask used to specify the subset.
 * @param index Index into original sequence.
 * @return Index into sequence of elements with positive bit in mask.
 */
template <typename T>
size_t to_relative_index(T const& mask, size_t const index)
{
	if (index >= mask.size()) {
		throw std::out_of_range("mask to short");
	}

	if (!mask.test(index)) {
		throw std::invalid_argument("index not enabled in mask");
	}

	size_t relative = 0;
	for (size_t ii = 0; ii < index; ++ii) {
		relative += mask[ii];
	}

	return relative;
}

template <typename T>
size_t from_relative_index(T const& mask, size_t const index)
{
	if (index >= mask.count()) {
		throw std::out_of_range("mask to short");
	}

	size_t ii = 0;
	for (size_t relative = 0; ii < mask.size(); ++ii) {
		relative += mask[ii];
		if (relative > index) {
			break;
		}
	}

	return ii;
}

} // namespace routing
} // namespace marocco
