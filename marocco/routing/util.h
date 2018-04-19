#pragma once

#include <map>
#include <stdexcept>

#include "hate/macros.h"

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
	if (HATE_UNLIKELY(index >= mask.size())) {
		throw std::out_of_range("mask to short");
	}

	/* store global-index (0 to mask.size()) to relative-index (0 to mask.count()) */
	static auto cache = std::make_unique<std::map<T, std::vector<size_t>>>();

	if (cache->find(mask) == cache->end()) {
		// no entry for this mask: create and fill
		std::vector<size_t> tmp;
		for (size_t relative = 0, ii = 0; ii < mask.size(); ++ii) {
			tmp.push_back(relative);
			relative += mask[ii];
		}
		(*cache)[mask] = tmp;
	}

	if (HATE_UNLIKELY(!mask.test(index))) {
		// TODO: check for *cache[mask][index-1] == [index]?
		throw std::invalid_argument("index not enabled in mask");
	}

	return cache->at(mask).at(index);
}

template <typename T>
size_t from_relative_index(T const& mask, size_t const index)
{
	/* store relative-index (0 to mask.count()) to global-index (0 to mask.size()) */
	static auto cache = std::make_unique<std::map<T, std::vector<size_t>>>();

	if (cache->find(mask) == cache->end()) {
		// no entry for this mask: create and fill
		std::vector<size_t> tmp;
		for (size_t ii = 0; ii < mask.size(); ++ii) {
			if (mask[ii]) {
				tmp.push_back(ii);
			}
		}
		(*cache)[mask] = tmp;
	}

	if (HATE_UNLIKELY(index >= mask.count())) {
		// TODO: check for *cache[mask].size()?
		throw std::out_of_range("mask to short");
	}

	return cache->at(mask).at(index);
}

} // namespace routing
} // namespace marocco

