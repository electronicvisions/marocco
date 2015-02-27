#pragma once

#include <tuple>
#include <iostream>

// ostream operator for std tuples. retrieved from:
// http://cpplove.blogspot.de/2012/07/printing-tuples.html

namespace marocco {
namespace routing {

// Define a type which holds an unsigned integer value
template <std::size_t>
struct int_
{
};

template <class Tuple, size_t Pos>
std::ostream& print_tuple(std::ostream& out, const Tuple& t, int_<Pos>)
{
	out << std::get<std::tuple_size<Tuple>::value - Pos>(t) << ',';
	return print_tuple(out, t, int_<Pos - 1>());
}

template <class Tuple>
std::ostream& print_tuple(std::ostream& out, const Tuple& t, int_<1>)
{
	return out << std::get<std::tuple_size<Tuple>::value - 1>(t);
}

template <class... Args>
std::ostream& operator<<(std::ostream& out, const std::tuple<Args...>& t)
{
	out << '(';
	print_tuple(out, t, int_<sizeof...(Args)>());
	return out << ')';
}

} // namespace routing
} // namespace marocco
