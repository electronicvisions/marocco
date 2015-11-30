#pragma once

#include <boost/serialization/nvp.hpp>

#include "marocco/tbb.h"

namespace marocco {

template <typename Key, typename T>
class AssociativeResult
{
public:
	typedef Key key_type;
	typedef T result_type;
	typedef tbb::concurrent_unordered_map<key_type, result_type> container_type;

	result_type const& at(key_type const& key) const
	{
		return _mapping.at(key);
	}

#if !defined(PYPLUSPLUS)
	result_type& at(key_type const& key)
	{
		return _mapping.at(key);
	}
#endif

	result_type& operator[](key_type const& key)
	{
		return _mapping[key];
	}

	bool exists(key_type const& key) const
	{
		return _mapping.find(key) != _mapping.end();
	}

	size_t size() const
	{
		return _mapping.size();
	}

	typename container_type::const_iterator find(key_type const& key) const
	{
		return _mapping.find(key);
	}

	typename container_type::iterator find(key_type const& key)
	{
		return _mapping.find(key);
	}

	typename container_type::iterator begin()
	{
		return _mapping.begin();
	}

	typename container_type::const_iterator begin() const
	{
		return _mapping.begin();
	}

	typename container_type::iterator end()
	{
		return _mapping.end();
	}

	typename container_type::const_iterator end() const
	{
		return _mapping.end();
	}

protected:
	friend class boost::serialization::access;
	template <typename Archiver>
	void serialize(Archiver& ar, unsigned int const /*version*/)
	{
		ar& boost::serialization::make_nvp("mapping", _mapping);
	}

	container_type _mapping;
};

} // namespace marocco
