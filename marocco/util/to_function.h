#pragma once

#include <functional>

namespace marocco {

namespace detail {

template <typename FunctionT>
struct function_type
{
};

template <typename ReturnT, typename... Args>
struct function_type<ReturnT(*)(Args...)>
{
	typedef std::function<ReturnT(Args...)> type;
};

template <typename ClassT, typename ReturnT, typename... Args>
struct function_type<ReturnT(ClassT::*)(Args...)>
{
	typedef std::function<ReturnT(Args...)> type;
};

template <typename ClassT, typename ReturnT, typename... Args>
struct function_type<ReturnT(ClassT::*)(Args...) const>
{
	typedef std::function<ReturnT(Args...)> type;
};

} // namespace detail

template <typename Lambda>
typename detail::function_type<decltype(&Lambda::operator())>::type
to_function(Lambda const& lambda)
{
	return lambda;
}

template <typename F, typename... Args>
typename detail::function_type<F>::type to_function(F&& f, Args&&... args)
{
	return std::bind(std::forward<F>(f), std::forward<Args>(args)...);
}

} // namespace marocco
