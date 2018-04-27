#pragma once

#include <unordered_map>
#include <stdexcept>

#include "hate/macros.h"

#include <boost/functional/hash.hpp>
#include <boost/dynamic_bitset.hpp>
#include <boost/unordered_map.hpp>

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
	static auto cache =
	    std::make_unique<boost::unordered_map<T, std::vector<size_t> > >();

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
	static auto cache =
	    std::make_unique<boost::unordered_map<T, std::vector<size_t> > >();

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

#ifdef BOOST_DYNAMIC_BITSET_DONT_USE_FRIENDS
// provide hash_value function if we can access the private members
namespace boost {
	template <typename B, typename A>
		std::size_t hash_value(const dynamic_bitset<B, A>& a) {
			std::size_t res = hash_value(a.m_num_bits);
			hash_combine(res, hash_value(a.m_bits));
			return res;
		}
}
#else
#error BOOST_DYNAMIC_BITSET_DONT_USE_FRIENDS not defined. \
       Need to access access the private member to provide hash_value function!
#endif
