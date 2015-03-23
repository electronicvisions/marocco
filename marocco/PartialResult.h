#pragma once

#include <map>
#include <cstdlib>
#include <stdexcept>

#include <boost/serialization/nvp.hpp>

#include "marocco/Logger.h"
#include "marocco/tbb.h"

namespace marocco {

template<typename Derived, typename Index, size_t Stage>
class PartialResult
{
public:
	typedef typename Derived::hardware_type hardware_type;
	typedef typename Derived::result_type result_type;

	typedef Index index_type;
	typedef tbb::concurrent_unordered_map<index_type, result_type> map_t;

	// designates the stage of this PartialResult, in which it has to be
	// derived to get to an complete Result.
	static size_t const stage = Stage;

	result_type const& at(index_type const& idx) const
	{
		return _mapping.at(idx);
	}
#if !defined(PYPLUSPLUS)
	result_type& at(index_type const& idx)
	{
		return _mapping.at(idx);
	}
#endif

	result_type&       operator[] (index_type const& idx)
	{
		return _mapping[idx];
	}

	void insert(index_type const& idx, result_type const& v)
	{
		std::pair<typename map_t::iterator, bool> ins =
			_mapping.insert(std::make_pair(idx, v));
		if (!ins.second)
			throw std::runtime_error("insertion failure"); \
	}

	bool exists(index_type const& idx) const
	{
		return _mapping.find(idx) != _mapping.end();
	}

	size_t size() const
	{
		return _mapping.size();
	}

	typename map_t::const_iterator find(index_type const& idx) const
	{
		return _mapping.find(idx);
	}

	typename map_t::iterator find(index_type const& idx)
	{
		return _mapping.find(idx);
	}

	typename map_t::iterator       begin()       { return _mapping.begin(); }
	typename map_t::const_iterator begin() const { return _mapping.begin(); }
	typename map_t::iterator       end()         { return _mapping.end(); }
	typename map_t::const_iterator end() const   { return _mapping.end(); }

protected:
	friend class boost::serialization::access;
	template<typename Archiver>
	void serialize(Archiver& ar, unsigned int const /*version*/)
	{
		ar & boost::serialization::make_nvp("mapping", _mapping);
	}

	map_t _mapping;
};

template<typename Derived, typename Index, size_t Stage>
size_t const PartialResult<Derived, Index, Stage>::stage;

} // namespace marocco
