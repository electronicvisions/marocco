#pragma once

#include <type_traits>
#include <boost/property_map/property_map.hpp>

namespace marocco {

template <typename ReturnT, typename KeyT>
class function_property_map
	: public boost::put_get_helper<ReturnT, function_property_map<ReturnT, KeyT> >
{
public:
	typedef KeyT key_type;
	typedef ReturnT reference;
	typedef typename std::remove_cv<typename std::remove_reference<ReturnT>::type>::type value_type;
	typedef typename std::conditional<std::is_reference<ReturnT>::value &&
	                                      !std::is_const<ReturnT>::value,
	                                  boost::lvalue_property_map_tag,
	                                  boost::readable_property_map_tag>::type category;
	typedef std::function<reference(key_type const&)> function_type;

	function_property_map(function_type handler) : m_handler(handler)
	{
	}

	reference operator[](key_type const& key) const
	{
		return m_handler(key);
	}

private:
	function_type m_handler;
}; // function_property_map

template <typename ReturnT, typename KeyT>
function_property_map<ReturnT, KeyT>
make_function_property_map(std::function<ReturnT(KeyT const&)> handler)
{
	return function_property_map<ReturnT, KeyT>(handler);
}

} // namespace marocco
