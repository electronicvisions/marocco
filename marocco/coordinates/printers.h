#include "marocco/coordinates/L1Route.h"
#include "marocco/coordinates/L1RouteTree.h"
#include "marocco/coordinates/LogicalNeuron.h"

namespace marocco {

namespace detail {

template <typename T>
struct pretty_printer
{
	T const& what;
	size_t indent;
};

} // namespace detail

template <typename T>
detail::pretty_printer<T> pretty_printed(T const& what, size_t indent = 0)
{
	return detail::pretty_printer<T>{what, indent};
}

namespace detail {

std::ostream& operator<<(std::ostream& os, pretty_printer<L1Route> pr);
std::ostream& operator<<(std::ostream& os, pretty_printer<L1RouteTree> pr);
std::ostream& operator<<(std::ostream& os, pretty_printer<LogicalNeuron> pr);

} // namespace detail

} // namespace marocco
