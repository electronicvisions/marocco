#pragma once

#include <pywrap/compat/tuple.hpp>
#include <memory>

#include "marocco/BaseResult.h"

namespace marocco {

namespace detail {

template<typename ... Ts>
struct pack {};

template<typename Pack, size_t N=0>
struct assert_stages;

template<size_t N>
struct assert_stages<pack<>, N> {};

template<typename Result, typename ... Results, size_t N>
struct assert_stages<pack<Result, Results...>, N> :
	public assert_stages<pack<Results...>, N+1>
{
	static_assert(Result::stage == N, "subresult stage mismatch");
};

} // namespace detail

template<typename ... PartialResults>
class ResultInterface :
	public BaseResult,
	public detail::assert_stages<detail::pack<PartialResults...>>
{
private:
	typedef std::tuple<std::unique_ptr<PartialResults>...> tuple;
	tuple mResults;

public:
	template<size_t N, typename ... Ts>
	friend
	typename std::tuple_element<N, std::tuple<std::unique_ptr<Ts>...>>::type::element_type&
	get(ResultInterface<Ts ...>& r);

	template<size_t N, typename ... Ts>
	friend
	typename std::tuple_element<N, std::tuple<std::unique_ptr<Ts>...>>::type::element_type const&
	get(ResultInterface<Ts ...> const& r);

	template<size_t N, typename ... Ts>
	friend
	void set(ResultInterface<Ts ...>& r,
			 //typename std::tuple_element<N, std::tuple<std::unique_ptr<Ts>...>>::type::element_type&& ptr);
		 std::unique_ptr<typename std::tuple_element<N, std::tuple<std::unique_ptr<Ts>...>>::type::element_type>&& ptr);

	enum : size_t { size = std::tuple_size<tuple>::value };

};

template<size_t N, typename ... Ts>
typename std::tuple_element<N, std::tuple<std::unique_ptr<Ts>...>>::type::element_type&
get(ResultInterface<Ts ...>& r)
{
	return *std::get<N>(r.mResults);
}

template<size_t N, typename ... Ts>
typename std::tuple_element<N, std::tuple<std::unique_ptr<Ts>...>>::type::element_type const&
get(ResultInterface<Ts ...> const& r)
{
	return *std::get<N>(r.mResults);
}

template<size_t N, typename ... Ts>
void set(ResultInterface<Ts ...>& r,
		 std::unique_ptr<typename std::tuple_element<N, std::tuple<std::unique_ptr<Ts>...>>::type::element_type>&& ptr)
{
	std::get<N>(r.mResults) = std::move(ptr);
}

} // namespace marocco
