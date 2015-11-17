#pragma once

#include <iterator>
#include <vector>

#include <boost/iterator/indirect_iterator.hpp>

namespace marocco {

namespace detail {

/**
 * @brief Wrapper that allows to consume iterators in chunks.
 * @see marocco::chunked
 */
template <typename InputIt>
class chunked
{
	typedef typename std::remove_reference<InputIt>::type underlying_iterator;

	class iterator;

	class chunk
	{
		typedef std::vector<underlying_iterator> container_type;
		typedef boost::indirect_iterator<typename container_type::iterator> indirect_iterator;

		size_t idx = 0;
		size_t sz;
		container_type iterators;

		friend class iterator;

		chunk(size_t size) : sz(size)
		{
			iterators.reserve(sz);
		}

		underlying_iterator fill_from(underlying_iterator first, underlying_iterator last)
		{
			iterators.clear();
			for (size_t ii = 0; ii < sz && first != last; ++ii, ++first) {
				iterators.push_back(first);
			}
			return first;
		}

	public:
		bool empty() const
		{
			return iterators.empty();
		}

		size_t index() const
		{
			return idx;
		}

		size_t size() const
		{
			return iterators.size();
		}

		indirect_iterator begin()
		{
			return {iterators.begin()};
		}

		indirect_iterator end()
		{
			return {iterators.end()};
		}
	};

	class iterator : public std::iterator<std::input_iterator_tag, chunk>
	{
		underlying_iterator it;
		underlying_iterator end;
		chunk current_chunk;

	public:
		iterator(underlying_iterator const& first, underlying_iterator const& last, size_t chunk_size)
			: it(first), end(last), current_chunk(chunk_size)
		{
			it = current_chunk.fill_from(it, end);
		}

		iterator& operator++()
		{
			if (!current_chunk.empty()) {
				it = current_chunk.fill_from(it, end);
				++current_chunk.idx;
			}
			return *this;
		}

		iterator operator++(int)
		{
			iterator tmp(*this);
			++*this;
			return tmp;
		}

		bool operator==(const iterator& other) const
		{
			// iterators with empty chunks always compare equal
			return current_chunk.empty() == other.current_chunk.empty() &&
				!(!current_chunk.empty() && it != other.it);
			/*    | == | != |
			 *  --+----+----+	#: full
			 *  ##| A  | AB |	_: empty
			 *  #_|    |  B |	A: first condition
			 *  _#|    |    |	B: second condition
			 *  __| A  | A  |
			 */
		}

		bool operator!=(const iterator& other) const
		{
			return !(*this == other);
		}

		chunk& operator*()
		{
			return current_chunk;
		}

	}; // iterator

	iterator begin_;
	iterator end_;

public:
	chunked(underlying_iterator first, underlying_iterator last, size_t chunk_size)
		: begin_{first, last, chunk_size}, end_{last, last, chunk_size}
	{
	}

	iterator begin()
	{
		return begin_;
	}

	iterator end()
	{
		return end_;
	}
}; // chunked

namespace adl_kludge {

using std::begin;
template <typename T>
auto begin_(T&& x) -> decltype(begin(std::declval<T>()))
{
	return begin(std::forward<T>(x));
}

} // namespace adl_kludge

template <typename T>
struct iterator_of
{
	using type = decltype(adl_kludge::begin_(std::declval<T&>()));
};

} // namespace detail

/**
 * Return an iterator over chunks of the underlying iterator.
 */
template <typename InputIt>
auto chunked(InputIt first, InputIt last, size_t chunk_size) -> detail::chunked<InputIt>
{
	return {first, last, chunk_size};
}

template <typename Container>
auto chunked(Container&& cont, size_t chunk_size)
	-> detail::chunked<typename detail::iterator_of<Container>::type>
{
	return {cont.begin(), cont.end(), chunk_size};
}

} // namespace marocco
