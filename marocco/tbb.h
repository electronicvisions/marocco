#pragma once

#include <algorithm>
#include <tbb/concurrent_unordered_map.h>
#include <boost/serialization/concurrent_unordered_map.h>
#include "hal/Coordinate/HMFGeometry.h"

namespace tbb {

template<>
struct tbb_hash<HMF::Coordinate::HICANNGlobal>
{
	typedef HMF::Coordinate::HICANNGlobal type;
	size_t operator() (type const& key) const
	{
#if !defined(PYPLUSPLUS)
		static_assert(sizeof(type)>0, "just to make sure, "
					  "this part is not compiled by py++");
		static const std::hash<type> hash{};
		return hash(key);
#else
		return 42;
#endif
	}
};


template<class Type, class Key, class Hash, class Compare, class Allocator>
inline bool
operator==(concurrent_unordered_map<Key, Type, Hash, Compare, Allocator> const& a,
		   concurrent_unordered_map<Key, Type, Hash, Compare, Allocator> const& b)
{
	return std::equal(a.begin(), a.end(), b.begin());
}

template<class Type, class Key, class Hash, class Compare, class Allocator>
inline bool
operator!=(concurrent_unordered_map<Key, Type, Hash, Compare, Allocator> const& a,
		   concurrent_unordered_map<Key, Type, Hash, Compare, Allocator> const& b)
{
	return !(a==b);
}

template<class Type, class Key, class Hash, class Compare, class Allocator>
inline bool
operator<(concurrent_unordered_map<Key, Type, Hash, Compare, Allocator> const& a,
		  concurrent_unordered_map<Key, Type, Hash, Compare, Allocator> const& b)
{
	return std::lexicographical_compare(a.begin(), a.end(),
										b.begin(), b.end());
}

template<class Type, class Key, class Hash, class Compare, class Allocator>
inline bool
operator>(concurrent_unordered_map<Key, Type, Hash, Compare, Allocator> const& a,
		  concurrent_unordered_map<Key, Type, Hash, Compare, Allocator> const& b)
{
	return b < a;
}

template<class Type, class Key, class Hash, class Compare, class Allocator>
inline bool
operator<=(concurrent_unordered_map<Key, Type, Hash, Compare, Allocator> const& a,
		   concurrent_unordered_map<Key, Type, Hash, Compare, Allocator> const& b)
{
	return !(a>b);
}

template<class Type, class Key, class Hash, class Compare, class Allocator>
inline bool
operator>=(concurrent_unordered_map<Key, Type, Hash, Compare, Allocator> const& a,
		   concurrent_unordered_map<Key, Type, Hash, Compare, Allocator> const& b)
{
	return !(a<b);
}

} // namespace tbb
