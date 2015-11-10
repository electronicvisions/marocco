#pragma once

#include <iterator>
#include <utility>

#include "marocco/util/algorithm.h"

namespace marocco {

namespace detail {

template <typename T, T first, T... rest>
struct enum_min
{
	typedef typename std::underlying_type<T>::type underlying_type;
	static constexpr underlying_type value = static_cast<underlying_type>(first);
};

template <typename T, T first, T next, T... rest>
struct enum_min<T, first, next, rest...>
{
	typedef typename std::underlying_type<T>::type underlying_type;
	static constexpr underlying_type value =
		enum_min<T, (static_cast<underlying_type>(first) <= static_cast<underlying_type>(next))
		            ? first : next, rest...>::value;
};

template <typename T, T first, T... rest>
struct enum_max
{
	typedef typename std::underlying_type<T>::type underlying_type;
	static constexpr underlying_type value = static_cast<underlying_type>(first);
};

template <typename T, T first, T next, T... rest>
struct enum_max<T, first, next, rest...>
{
	typedef typename std::underlying_type<T>::type underlying_type;
	static constexpr underlying_type value =
		enum_max<T, (static_cast<underlying_type>(first) >= static_cast<underlying_type>(next))
		            ? first : next, rest...>::value;
};

} // namespace detail

template <typename T, T... args>
class enum_iterator : public std::iterator<std::input_iterator_tag, T const>
{
public:
	typedef typename std::underlying_type<T>::type underlying_type;
	static constexpr T items[] = {args...};
	static constexpr size_t size = sizeof...(args);
	static constexpr underlying_type begin = detail::enum_min<T, args...>::value;
	static constexpr underlying_type end = detail::enum_max<T, args...>::value + 1;
	static constexpr underlying_type min = detail::enum_min<T, args...>::value;
	static constexpr underlying_type max = detail::enum_max<T, args...>::value;

	enum_iterator() : pos(size)
	{
	}

	enum_iterator(size_t pos_) : pos(pos_)
	{
	}

	enum_iterator(T val) : pos(algorithm::index_of(std::begin(items), std::end(items), val))
	{
	}

	T const& operator*() const
	{
		return items[pos];
	}

	enum_iterator& operator++()
	{
		++pos;
		return *this;
	}

	enum_iterator operator++(int)
	{
		enum_iterator tmp(*this);
		++pos;
		return tmp;
	}

	bool operator==(enum_iterator const& other)
	{
		return other.pos == pos;
	}

	bool operator!=(enum_iterator const& other)
	{
		return other.pos != pos;
	}

private:
	size_t pos;
};

template <typename T, T... args>
constexpr T enum_iterator<T, args...>::items[];

} // namespace marocco
